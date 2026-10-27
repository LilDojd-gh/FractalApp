#include "MainWindow.h"

#include <wx/clrpicker.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <algorithm>

#include "Canvas2D.h"
#include "Canvas3D.h"
#include "ControlPanel.h"
#include "Ids.h"
#include "SettingsStorage.h"
#include "Util.h"

using namespace fractal;

namespace {
const char* typeName(FractalType t) {
    switch (t) {
        case FractalType::Julia: return "Жюлиа";
        case FractalType::DiamondSquare: return "Diamond-Square";
        default: return "Мандельброт";
    }
}
}  // namespace

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, U8("Построение фрактальных поверхностей — FractalSurf"), wxDefaultPosition, wxSize(1360, 840)) {
    SetMinSize(wxSize(980, 640));
    CreateStatusBar(3);
    const int widths[3] = {-3, -4, -1};
    SetStatusWidths(3, widths);

    buildMenu();
    buildLayout();

    // Настройки прошлого сеанса.
    wxFileName dir(wxStandardPaths::Get().GetUserDataDir(), "");
    wxFileName::Mkdir(dir.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    settingsPath_ = dir.GetPath() + wxFileName::GetPathSeparator() + "settings.ini";
    const FractalParams saved = SettingsStorage::load(settingsPath_.utf8_string());
    panel_->applyParams(saved);
    controller_.setParams(saved);
    switchPage(saved.mode3D);
    canvas3D_->setWireframe(saved.wireframe);

    // События элементов приходят по иерархии окон; различаем их по идентификатору.
    Bind(wxEVT_BUTTON, &MainWindow::OnRenderClicked, this, ID_RENDER);
    Bind(wxEVT_MENU, &MainWindow::OnRenderClicked, this, ID_RENDER);
    Bind(wxEVT_BUTTON, &MainWindow::OnSaveClicked, this, ID_SAVE);
    Bind(wxEVT_MENU, &MainWindow::OnSaveClicked, this, ID_SAVE);
    Bind(wxEVT_BUTTON, &MainWindow::OnResetClicked, this, ID_RESET);
    Bind(wxEVT_MENU, &MainWindow::OnResetClicked, this, ID_RESET);
    Bind(wxEVT_MENU, &MainWindow::OnResetView, this, ID_RESET_VIEW);
    Bind(wxEVT_MENU, &MainWindow::OnHelp, this, wxID_HELP);
    Bind(wxEVT_MENU, &MainWindow::OnHelp, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MainWindow::OnExit, this, wxID_EXIT);
    Bind(wxEVT_CHOICE, &MainWindow::OnFractalChanged, this, ID_TYPE);
    Bind(wxEVT_CHECKBOX, &MainWindow::OnModeToggled, this, ID_MODE3D);
    for (int id : {ID_PALETTE, ID_INTERP}) Bind(wxEVT_CHOICE, &MainWindow::OnAppearanceChanged, this, id);
    for (int id : {ID_CYCLES, ID_HEIGHT}) Bind(wxEVT_SLIDER, &MainWindow::OnAppearanceChanged, this, id);
    Bind(wxEVT_RADIOBOX, &MainWindow::OnAppearanceChanged, this, ID_RENDER_MODE);
    Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent&) { wxCommandEvent e; OnAppearanceChanged(e); }, ID_INSIDE);
    Bind(wxEVT_SCROLL_CHANGED, &MainWindow::OnZoomChanged, this, ID_ZOOM);
    // Enter в полях ввода запускает рендеринг.
    for (int id : {ID_ITERATIONS, ID_JULIA_RE, ID_JULIA_IM, ID_SEED})
        Bind(wxEVT_TEXT_ENTER, &MainWindow::OnRenderClicked, this, id);
    Bind(wxEVT_CLOSE_WINDOW, &MainWindow::OnClose, this);
    Bind(wxEVT_CHAR_HOOK, &MainWindow::OnCharHook, this);

    canvas2D_->onZoom = [this](int x, int y, double k) { OnCanvasZoom(x, y, k); };
    canvas2D_->onPan = [this](int dx, int dy) { OnCanvasPan(dx, dy); };
    canvas2D_->onResized = [this] { OnCanvasResized(); };
    canvas3D_->onCameraChanged = [this] { update3DStatus(); };

    SetDefaultItem(panel_->btnRender_);  // Enter = «Рендеринг»
    Centre();
    CallAfter([this] { renderNow(); });   // первый кадр — когда окно уже получило размеры
}

void MainWindow::buildMenu() {
    auto* file = new wxMenu;
    file->Append(ID_RENDER, U8("Рендеринг\tCtrl+R"));
    file->Append(ID_SAVE, U8("Сохранить изображение…\tCtrl+S"));
    file->AppendSeparator();
    file->Append(wxID_EXIT, U8("Выход\tCtrl+Q"));
    auto* view = new wxMenu;
    view->Append(ID_RESET_VIEW, U8("Сбросить вид\tCtrl+0"));
    view->Append(ID_RESET, U8("Сбросить все параметры"));
    auto* help = new wxMenu;
    help->Append(wxID_HELP, U8("Справка\tF1"));
    help->Append(wxID_ABOUT, U8("О программе"));
    auto* bar = new wxMenuBar;
    bar->Append(file, U8("Файл"));
    bar->Append(view, U8("Вид"));
    bar->Append(help, U8("Справка"));
    SetMenuBar(bar);
}

void MainWindow::buildLayout() {
    auto* root = new wxPanel(this);
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    book_ = new wxSimplebook(root, wxID_ANY);
    canvas2D_ = new Canvas2D(book_);
    canvas3D_ = new Canvas3D(book_, controller_);
    book_->AddPage(canvas2D_, "2D", true);
    book_->AddPage(canvas3D_, "3D", false);
    panel_ = new ControlPanel(root);
    sizer->Add(book_, 1, wxEXPAND | wxALL, 4);   // холст растягивается вместе с окном
    sizer->Add(panel_, 0, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 4);  // панель — фиксированной ширины
    root->SetSizer(sizer);
    auto* top = new wxBoxSizer(wxVERTICAL);
    top->Add(root, 1, wxEXPAND);
    SetSizer(top);
}

void MainWindow::switchPage(bool mode3D) { book_->SetSelection(mode3D ? 1 : 0); }

wxString MainWindow::overlayText() const {
    const FractalParams& p = controller_.params();
    if (p.type == FractalType::DiamondSquare)
        return wxString::Format(U8("Diamond-Square %dx%d    H = %.2f    seed = %u"), controller_.heightMap().width(),
                                controller_.heightMap().height(), p.roughness, p.seed);
    return U8(controller_.describeView());
}

// ---------------------------------------------------------------------------
void MainWindow::renderNow() {
    if (busy_) return;
    FractalParams p;
    wxString err;
    if (!panel_->readParams(p, err)) {
        SetStatusText(err, 0);
        wxBell();
        return;
    }
    busy_ = true;
    {
        wxBusyCursor wait;
        controller_.setParams(p);
        if (p.mode3D) {
            controller_.setResolution(513, 513);  // для рельефа достаточно квадратной карты фиксированного размера
        } else {
            const wxSize s = canvas2D_->GetClientSize();
            controller_.setResolution(s.x, s.y);
        }
        recomputeAndShow();
    }
    busy_ = false;
}

void MainWindow::recomputeAndShow() {
    wxStopWatch sw;
    controller_.recompute();
    lastMs_ = sw.Time();
    showResult();
    updateStatus();
}

void MainWindow::showResult() {
    const FractalParams& p = controller_.params();
    if (p.mode3D) {
        controller_.buildMesh();
        canvas3D_->setWireframe(p.wireframe);
        canvas3D_->meshChanged();
    } else {
        canvas2D_->setImage(controller_.render2D(), overlayText());
    }
}

void MainWindow::updateStatus() {
    const FractalParams& p = controller_.params();
    SetStatusText(wxString::Format(U8("%s   |   %dx%d   |   %ld мс"), U8(typeName(p.type)),
                                   controller_.heightMap().width(), controller_.heightMap().height(), lastMs_), 0);
    if (p.mode3D) update3DStatus();
    else SetStatusText(overlayText(), 1);
    SetStatusText(U8("Готово"), 2);
}

void MainWindow::update3DStatus() {
    const Camera3D& c = controller_.camera();
    SetStatusText(wxString::Format(U8("Yaw: %.0f°   Pitch: %.0f°   Дистанция: %.2f   Вершин: %d   Режим: %s"), c.yaw(),
                                   c.pitch(), c.distance(), static_cast<int>(controller_.mesh().vertices().size()),
                                   controller_.params().wireframe ? "Wireframe" : "Shaded"), 1);
}

// ---------------------------------------------------------------------------
void MainWindow::OnRenderClicked(wxCommandEvent&) { renderNow(); }

void MainWindow::OnFractalChanged(wxCommandEvent&) {
    // При смене фрактала подбираем подходящую палитру и сразу строим новое изображение.
    const FractalType t = panel_->selectedType();
    FractalParams p;
    wxString err;
    panel_->readParams(p, err);  // остальные поля панели сохраняются; при ошибке ввода берутся значения по умолчанию
    p.type = t;
    p.presetId = FractalParams::defaultPresetFor(t);
    p.interp = InterpMode::Linear;
    p.paletteCycles = (t == FractalType::DiamondSquare) ? 1.0f : 30.0f;
    panel_->applyParams(p);
    renderNow();  // новый тип — новый генератор с видом по умолчанию
}

void MainWindow::OnModeToggled(wxCommandEvent&) {
    const bool m3 = panel_->check3D_->GetValue();
    switchPage(m3);
    renderNow();  // для 3D строится карта фиксированного размера, для 2D — по размеру холста
}

void MainWindow::OnAppearanceChanged(wxCommandEvent&) {
    FractalParams p = controller_.params();
    panel_->readAppearance(p);
    panel_->setPalettePreview(p.presetId, p.interp);
    controller_.updateAppearance(p);
    canvas3D_->setWireframe(p.wireframe);
    if (controller_.hasData()) {
        showResult();      // перекраска без пересчёта фрактала
        updateStatus();
    }
}

void MainWindow::OnZoomChanged(wxScrollEvent&) {
    controller_.setZoomFactor(panel_->zoomFactor());
    renderNow();
}

void MainWindow::OnResetView(wxCommandEvent&) {
    controller_.resetView();
    panel_->setZoomFactor(1.0);
    renderNow();
}

void MainWindow::OnResetClicked(wxCommandEvent&) {
    wxMessageDialog dlg(this, U8("Вернуть все параметры к значениям по умолчанию? Это действие нельзя отменить."),
                        U8("Сброс параметров"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    dlg.SetYesNoLabels(U8("Сбросить"), U8("Отмена"));
    if (dlg.ShowModal() != wxID_YES) return;
    const FractalParams def;
    panel_->applyParams(def);
    controller_.setParams(def);
    controller_.resetView();
    controller_.camera().reset();
    switchPage(def.mode3D);
    renderNow();
}

void MainWindow::OnSaveClicked(wxCommandEvent&) {
    if (!controller_.hasData()) {
        wxMessageBox(U8("Сначала постройте изображение (кнопка «Рендеринг»)."), U8("Сохранение"), wxOK | wxICON_INFORMATION, this);
        return;
    }
    wxFileDialog dlg(this, U8("Сохранить изображение"), "", "fractal",
                     U8("PNG (*.png)|*.png|BMP (*.bmp)|*.bmp"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    wxString path = dlg.GetPath();
    wxString ext = wxFileName(path).GetExt().Lower();
    if (ext != "png" && ext != "bmp") {
        ext = dlg.GetFilterIndex() == 1 ? "bmp" : "png";
        path += "." + ext;
    }
    wxImage img;
    if (controller_.params().mode3D) {
        if (!canvas3D_->captureFrame(img)) {
            wxMessageBox(U8("Не удалось получить кадр 3D-сцены."), U8("Ошибка"), wxOK | wxICON_ERROR, this);
            return;
        }
    } else {
        img = toWxImage(controller_.render2D());
    }
    if (!img.IsOk() || !img.SaveFile(path, ext == "bmp" ? wxBITMAP_TYPE_BMP : wxBITMAP_TYPE_PNG)) {
        wxMessageBox(U8("Не удалось сохранить файл:\n") + path, U8("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }
    SetStatusText(U8("Сохранено: ") + path, 2);
}

void MainWindow::OnCanvasZoom(int px, int py, double k) {
    if (busy_ || !controller_.supportsNavigation()) return;
    busy_ = true;
    controller_.zoomAt(px, py, k);
    panel_->setZoomFactor(controller_.zoomFactor());
    recomputeAndShow();
    busy_ = false;
}

void MainWindow::OnCanvasPan(int dx, int dy) {
    if (busy_ || !controller_.supportsNavigation()) return;
    busy_ = true;
    controller_.pan(dx, dy);
    recomputeAndShow();
    busy_ = false;
}

void MainWindow::OnCanvasResized() {
    if (busy_ || controller_.params().mode3D || !controller_.supportsNavigation() || !controller_.hasData()) return;
    const wxSize s = canvas2D_->GetClientSize();
    if (s.x == controller_.heightMap().width() && s.y == controller_.heightMap().height()) return;
    busy_ = true;
    controller_.setResolution(s.x, s.y);
    recomputeAndShow();
    busy_ = false;
}

void MainWindow::OnHelp(wxCommandEvent&) {
    wxMessageBox(U8("FractalSurf — построение фрактальных поверхностей\n\n"
                    "Мандельброт / Жюлиа:\n"
                    "  • колесо мыши — масштаб относительно курсора\n"
                    "  • перетаскивание левой кнопкой — сдвиг области просмотра\n\n"
                    "3D-режим:\n"
                    "  • перетаскивание левой кнопкой — вращение камеры\n"
                    "  • колесо мыши — приближение и удаление\n\n"
                    "Клавиши: Enter / Ctrl+R — рендеринг, Ctrl+S — сохранить изображение,\n"
                    "Ctrl+0 — сбросить вид, Esc — вернуть фокус на изображение, F1 — справка."),
                 U8("Справка"), wxOK | wxICON_INFORMATION, this);
}

void MainWindow::OnExit(wxCommandEvent&) { Close(); }

void MainWindow::OnClose(wxCloseEvent& e) {
    FractalParams p;
    wxString err;
    if (panel_->readParams(p, err)) SettingsStorage::save(p, settingsPath_.utf8_string());
    e.Skip();  // продолжить закрытие окна
}

void MainWindow::OnCharHook(wxKeyEvent& e) {
    if (e.GetKeyCode() == WXK_ESCAPE) {
        (controller_.params().mode3D ? static_cast<wxWindow*>(canvas3D_) : static_cast<wxWindow*>(canvas2D_))->SetFocus();
        return;
    }
    e.Skip();
}

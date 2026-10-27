#include "ControlPanel.h"

#include <wx/clrpicker.h>
#include <wx/dcbuffer.h>
#include <wx/spinctrl.h>
#include <wx/valtext.h>

#include <cmath>

#include "ColorPalette.h"
#include "Ids.h"
#include "Util.h"

using namespace fractal;

// ---------------------------------------------------------------------------
/// Полоса-предпросмотр палитры (рисуется по одному столбцу пикселей).
class PalettePreview : public wxPanel {
public:
    explicit PalettePreview(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 22)) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(100, 22));
        Bind(wxEVT_PAINT, &PalettePreview::OnPaint, this);
    }
    void setPalette(const ColorPalette& p) {
        palette_ = p;
        Refresh();
    }

private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        const wxSize s = GetClientSize();
        for (int x = 0; x < s.x; ++x) {
            const Rgb c = palette_.colorAt(s.x > 1 ? static_cast<float>(x) / (s.x - 1) : 0.0f);
            dc.SetPen(wxPen(wxColour(c.r, c.g, c.b)));
            dc.DrawLine(x, 0, x, s.y);
        }
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(120, 120, 120)));
        dc.DrawRectangle(0, 0, s.x, s.y);
    }
    ColorPalette palette_;
};

// ---------------------------------------------------------------------------
namespace {
const int kZoomSteps = 200;  // положение ползунка v -> масштаб 2^(v/10): от x1 до x1 048 576

wxStaticText* hint(wxWindow* parent, const wxString& text) {
    auto* t = new wxStaticText(parent, wxID_ANY, text);
    t->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    return t;
}
}  // namespace

ControlPanel::ControlPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    // Группы параметров лежат в прокручиваемой области (на низких экранах ничего не обрезается),
    // кнопки закреплены внизу панели.
    auto* scroll = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    scroll->SetScrollRate(0, 12);
    auto* root = new wxBoxSizer(wxVERTICAL);  // содержимое прокручиваемой области
    const int pad = 6;
    auto row = [&](wxFlexGridSizer* g, wxWindow* parentBox, const wxString& label, wxWindow* ctrl) {
        g->Add(new wxStaticText(parentBox, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
        g->Add(ctrl, 1, wxEXPAND);
    };

    // ---------------- Параметры фрактала ----------------
    auto* g1 = new wxStaticBoxSizer(wxVERTICAL, scroll, U8("Параметры фрактала"));
    wxWindow* b1 = g1->GetStaticBox();
    choiceFractal_ = new wxChoice(b1, ID_TYPE);
    choiceFractal_->Append(U8("Множество Мандельброта"));
    choiceFractal_->Append(U8("Множество Жюлиа"));
    choiceFractal_->Append(U8("Diamond-Square (рельеф)"));
    choiceFractal_->SetToolTip(U8("Тип генерируемого фрактала"));

    spinIterations_ = new wxSpinCtrl(b1, ID_ITERATIONS, "500", wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 10, 5000, 500);
    spinIterations_->SetToolTip(U8("Предел итераций N_max (10…5000). Больше итераций — точнее граница, но дольше расчёт"));

    sliderZoom_ = new wxSlider(b1, ID_ZOOM, 0, 0, kZoomSteps);
    sliderZoom_->SetToolTip(U8("Масштаб. Также работает колесо мыши на изображении"));
    labelZoom_ = new wxStaticText(b1, wxID_ANY, "x1.0", wxDefaultPosition, wxSize(52, -1));

    auto* grid = new wxFlexGridSizer(2, pad, 8);
    grid->AddGrowableCol(1);
    row(grid, b1, U8("Тип фрактала:"), choiceFractal_);
    row(grid, b1, U8("Итераций:"), spinIterations_);
    grid->Add(new wxStaticText(b1, wxID_ANY, U8("Масштаб:")), 0, wxALIGN_CENTER_VERTICAL);
    {
        auto* h = new wxBoxSizer(wxHORIZONTAL);
        h->Add(sliderZoom_, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
        h->Add(labelZoom_, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
        grid->Add(h, 1, wxEXPAND);
    }

    // Константа c множества Жюлиа: фильтр допускает только символы числа.
    wxTextValidator numVal(wxFILTER_INCLUDE_CHAR_LIST);
    numVal.SetCharIncludes("0123456789+-.,eE");
    textJuliaRe_ = new wxTextCtrl(b1, ID_JULIA_RE, "-0.8", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER, numVal);
    textJuliaIm_ = new wxTextCtrl(b1, ID_JULIA_IM, "0.156", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER, numVal);
    textJuliaRe_->SetToolTip(U8("Действительная часть константы c (от -2.5 до 2.5)"));
    textJuliaIm_->SetToolTip(U8("Мнимая часть константы c (от -2.5 до 2.5)"));
    row(grid, b1, U8("Re(c):"), textJuliaRe_);
    row(grid, b1, U8("Im(c):"), textJuliaIm_);

    sliderRough_ = new wxSlider(b1, ID_ROUGHNESS, 80, 5, 100);
    sliderRough_->SetToolTip(U8("Параметр Херста H: меньше — более «рваный» рельеф, больше — более гладкий"));
    grid->Add(new wxStaticText(b1, wxID_ANY, U8("Шероховатость H:")), 0, wxALIGN_CENTER_VERTICAL);
    {
        auto* h = new wxBoxSizer(wxHORIZONTAL);
        labelRough_ = new wxStaticText(b1, wxID_ANY, "0.80", wxDefaultPosition, wxSize(52, -1));
        h->Add(sliderRough_, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
        h->Add(labelRough_, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
        grid->Add(h, 1, wxEXPAND);
    }
    spinSeed_ = new wxSpinCtrl(b1, ID_SEED, "42", wxDefaultPosition, wxDefaultSize,
                               wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 0, 2147483647, 42);
    spinSeed_->SetToolTip(U8("Начальное значение датчика случайных чисел: один seed — один и тот же рельеф"));
    row(grid, b1, U8("Seed:"), spinSeed_);
    choiceGrid_ = new wxChoice(b1, ID_GRID);
    for (int n = 6; n <= 10; ++n) choiceGrid_->Append(wxString::Format("%d x %d", (1 << n) + 1, (1 << n) + 1));
    choiceGrid_->SetToolTip(U8("Размер карты высот Diamond-Square (2^n + 1 узлов по стороне)"));
    row(grid, b1, U8("Размер карты:"), choiceGrid_);
    g1->Add(grid, 0, wxEXPAND | wxALL, pad);
    g1->Add(hint(b1, U8("Re(c), Im(c) — только для Жюлиа;\nH, Seed, размер — только для Diamond-Square")), 0, wxLEFT | wxBOTTOM, pad);
    root->Add(g1, 0, wxEXPAND | wxALL, 4);

    // ---------------- Цветовая палитра ----------------
    auto* g2 = new wxStaticBoxSizer(wxVERTICAL, scroll, U8("Цветовая палитра"));
    wxWindow* b2 = g2->GetStaticBox();
    auto* grid2 = new wxFlexGridSizer(2, pad, 8);
    grid2->AddGrowableCol(1);
    choicePalette_ = new wxChoice(b2, ID_PALETTE);
    for (int i = 0; i < ColorPalette::presetCount(); ++i) choicePalette_->Append(U8(ColorPalette::presetName(i)));
    choicePalette_->SetToolTip(U8("Готовый цветовой градиент. Смена палитры не требует пересчёта фрактала"));
    row(grid2, b2, U8("Палитра:"), choicePalette_);
    choiceInterp_ = new wxChoice(b2, ID_INTERP);
    choiceInterp_->Append(U8("Линейная"));
    choiceInterp_->Append(U8("Логарифмическая"));
    choiceInterp_->SetToolTip(U8("Способ интерполяции цвета между узлами палитры"));
    row(grid2, b2, U8("Интерполяция:"), choiceInterp_);
    pickerInside_ = new wxColourPickerCtrl(b2, ID_INSIDE, *wxBLACK);
    pickerInside_->SetToolTip(U8("Цвет точек, принадлежащих множеству"));
    row(grid2, b2, U8("Цвет «внутри»:"), pickerInside_);
    sliderCycles_ = new wxSlider(b2, ID_CYCLES, 30, 1, 100);
    sliderCycles_->SetToolTip(U8("Сколько раз палитра повторяется на диапазоне итераций"));
    grid2->Add(new wxStaticText(b2, wxID_ANY, U8("Повторов палитры:")), 0, wxALIGN_CENTER_VERTICAL);
    {
        auto* h = new wxBoxSizer(wxHORIZONTAL);
        labelCycles_ = new wxStaticText(b2, wxID_ANY, "30", wxDefaultPosition, wxSize(52, -1));
        h->Add(sliderCycles_, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
        h->Add(labelCycles_, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
        grid2->Add(h, 1, wxEXPAND);
    }
    g2->Add(grid2, 0, wxEXPAND | wxALL, pad);
    preview_ = new PalettePreview(b2);
    g2->Add(preview_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, pad);
    root->Add(g2, 0, wxEXPAND | wxALL, 4);

    // ---------------- 3D-поверхность ----------------
    auto* g3 = new wxStaticBoxSizer(wxVERTICAL, scroll, U8("3D-поверхность"));
    wxWindow* b3 = g3->GetStaticBox();
    check3D_ = new wxCheckBox(b3, ID_MODE3D, U8("Строить 3D-поверхность"));
    check3D_->SetToolTip(U8("Показать карту высот как трёхмерный рельеф. Мышь: вращение (левая кнопка), приближение (колесо)"));
    g3->Add(check3D_, 0, wxALL, pad);
    wxArrayString modes;
    modes.Add(U8("Каркас (Wireframe)"));
    modes.Add(U8("Заливка (Shaded)"));
    radioMode_ = new wxRadioBox(b3, ID_RENDER_MODE, U8("Режим отрисовки"), wxDefaultPosition, wxDefaultSize, modes, 1, wxRA_SPECIFY_COLS);
    g3->Add(radioMode_, 0, wxEXPAND | wxLEFT | wxRIGHT, pad);
    sliderHeight_ = new wxSlider(b3, ID_HEIGHT, 50, 10, 150);
    sliderHeight_->SetToolTip(U8("Масштаб высоты рельефа S_z"));
    auto* hh = new wxBoxSizer(wxHORIZONTAL);
    labelHeight_ = new wxStaticText(b3, wxID_ANY, "x0.50", wxDefaultPosition, wxSize(52, -1));
    hh->Add(new wxStaticText(b3, wxID_ANY, U8("Высота:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    hh->Add(sliderHeight_, 1, wxEXPAND);
    hh->Add(labelHeight_, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
    g3->Add(hh, 0, wxEXPAND | wxALL, pad);
    root->Add(g3, 0, wxEXPAND | wxALL, 4);

    scroll->SetSizer(root);
    scroll->FitInside();

    // ---------------- Кнопки (всегда видны) ----------------
    btnRender_ = new wxButton(this, ID_RENDER, U8("Рендеринг"));
    btnRender_->SetToolTip(U8("Построить фрактал с текущими параметрами (Enter)"));
    btnSave_ = new wxButton(this, ID_SAVE, U8("Сохранить изображение"));
    btnSave_->SetToolTip(U8("Сохранить область визуализации в PNG или BMP (Ctrl+S)"));
    btnReset_ = new wxButton(this, ID_RESET, U8("Сброс параметров"));
    btnReset_->SetToolTip(U8("Вернуть все параметры к значениям по умолчанию"));
    wxFont f = btnRender_->GetFont();
    f.SetWeight(wxFONTWEIGHT_BOLD);
    btnRender_->SetFont(f);

    auto* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(scroll, 1, wxEXPAND);
    outer->Add(btnRender_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);
    auto* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->Add(btnSave_, 1, wxRIGHT, 4);
    bottom->Add(btnReset_, 1);
    outer->Add(bottom, 0, wxEXPAND | wxALL, 4);
    SetSizer(outer);
    SetMinSize(wxSize(370, -1));
    Bind(wxEVT_SLIDER, &ControlPanel::OnSlider, this);
    applyParams(FractalParams{});
}

void ControlPanel::OnSlider(wxCommandEvent& e) {
    updateLabels();
    e.Skip();  // событие должно дойти до MainWindow
}

void ControlPanel::updateLabels() {
    labelZoom_->SetLabel(wxString::Format("x%s", [&] {
        const double z = zoomFactor();
        return z >= 1000 ? wxString::Format("%.3g", z) : wxString::Format("%.1f", z);
    }()));
    labelRough_->SetLabel(wxString::Format("%.2f", sliderRough_->GetValue() / 100.0));
    labelCycles_->SetLabel(wxString::Format("%d", sliderCycles_->GetValue()));
    labelHeight_->SetLabel(wxString::Format("x%.2f", sliderHeight_->GetValue() / 100.0));
}

double ControlPanel::zoomFactor() const { return std::pow(2.0, sliderZoom_->GetValue() / 10.0); }

void ControlPanel::setZoomFactor(double f) {
    const int v = static_cast<int>(std::lround(std::log2(std::max(f, 1.0)) * 10.0));
    sliderZoom_->SetValue(std::min(v, kZoomSteps));
    updateLabels();
    labelZoom_->SetLabel(wxString::Format(f >= 1000 ? "x%.3g" : "x%.1f", f));  // точное значение, а не по ползунку
}

FractalType ControlPanel::selectedType() const {
    const int s = choiceFractal_->GetSelection();
    return s == 1 ? FractalType::Julia : (s == 2 ? FractalType::DiamondSquare : FractalType::Mandelbrot);
}

void ControlPanel::enableFor(FractalType t) {
    const bool escape = t != FractalType::DiamondSquare;
    spinIterations_->Enable(escape);
    sliderZoom_->Enable(escape);
    labelZoom_->Enable(escape);
    sliderCycles_->Enable(escape);
    labelCycles_->Enable(escape);
    pickerInside_->Enable(escape);
    textJuliaRe_->Enable(t == FractalType::Julia);
    textJuliaIm_->Enable(t == FractalType::Julia);
    sliderRough_->Enable(!escape);
    labelRough_->Enable(!escape);
    spinSeed_->Enable(!escape);
    choiceGrid_->Enable(!escape);
}

void ControlPanel::setPalettePreview(int presetId, InterpMode m) {
    ColorPalette p;
    p.loadPreset(presetId);
    p.setInterpolation(m);
    preview_->setPalette(p);
}

bool ControlPanel::parseNumber(const wxString& text, double& value) {
    wxString t = text;
    t.Trim().Trim(false);
    t.Replace(",", ".");  // допускаем запятую как десятичный разделитель
    return !t.IsEmpty() && t.ToCDouble(&value) && std::isfinite(value);
}

void ControlPanel::readAppearance(FractalParams& p) const {
    p.presetId = std::max(0, choicePalette_->GetSelection());
    p.interp = choiceInterp_->GetSelection() == 1 ? InterpMode::Logarithmic : InterpMode::Linear;
    const wxColour c = pickerInside_->GetColour();
    p.insideColor = Rgb{static_cast<std::uint8_t>(c.Red()), static_cast<std::uint8_t>(c.Green()),
                        static_cast<std::uint8_t>(c.Blue())};
    p.paletteCycles = static_cast<float>(sliderCycles_->GetValue());
    p.mode3D = check3D_->GetValue();
    p.wireframe = radioMode_->GetSelection() == 0;
    p.heightScale = sliderHeight_->GetValue() / 100.0f;
}

bool ControlPanel::readParams(FractalParams& p, wxString& error) {
    p.type = selectedType();
    p.maxIterations = spinIterations_->GetValue();
    p.roughness = sliderRough_->GetValue() / 100.0f;
    p.seed = static_cast<std::uint32_t>(spinSeed_->GetValue());
    p.gridLevel = 6 + std::max(0, choiceGrid_->GetSelection());
    readAppearance(p);

    bool ok = true;
    auto check = [&](wxTextCtrl* t, double& dst, const wxString& name) {
        double v = 0;
        if (!parseNumber(t->GetValue(), v) || v < -2.5 || v > 2.5) {
            t->SetBackgroundColour(wxColour(255, 200, 200));
            if (ok) error = U8("Некорректное значение ") + name + U8(": нужно число от -2.5 до 2.5");
            ok = false;
        } else {
            t->SetBackgroundColour(wxNullColour);
            dst = v;
        }
        t->Refresh();
    };
    if (p.type == FractalType::Julia) {
        check(textJuliaRe_, p.juliaRe, "Re(c)");
        check(textJuliaIm_, p.juliaIm, "Im(c)");
    }
    return ok;
}

void ControlPanel::applyParams(const FractalParams& p) {
    choiceFractal_->SetSelection(static_cast<int>(p.type));
    spinIterations_->SetValue(p.maxIterations);
    textJuliaRe_->ChangeValue(wxString::FromCDouble(p.juliaRe, 4));
    textJuliaIm_->ChangeValue(wxString::FromCDouble(p.juliaIm, 4));
    textJuliaRe_->SetBackgroundColour(wxNullColour);
    textJuliaIm_->SetBackgroundColour(wxNullColour);
    sliderRough_->SetValue(static_cast<int>(std::lround(p.roughness * 100)));
    spinSeed_->SetValue(static_cast<int>(p.seed & 0x7fffffffu));
    choiceGrid_->SetSelection(p.gridLevel - 6);
    choicePalette_->SetSelection(p.presetId);
    choiceInterp_->SetSelection(p.interp == InterpMode::Logarithmic ? 1 : 0);
    pickerInside_->SetColour(wxColour(p.insideColor.r, p.insideColor.g, p.insideColor.b));
    sliderCycles_->SetValue(static_cast<int>(std::lround(p.paletteCycles)));
    check3D_->SetValue(p.mode3D);
    radioMode_->SetSelection(p.wireframe ? 0 : 1);
    sliderHeight_->SetValue(static_cast<int>(std::lround(p.heightScale * 100)));
    sliderZoom_->SetValue(0);
    updateLabels();
    setPalettePreview(p.presetId, p.interp);
    enableFor(p.type);
}

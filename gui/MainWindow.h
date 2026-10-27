// MainWindow.h — главное окно: холсты, панель управления, меню, строка состояния.
#pragma once
#include <wx/simplebook.h>
#include <wx/wx.h>

#include "FractalController.h"

class Canvas2D;
class Canvas3D;
class ControlPanel;

/**
 * Единственный класс интерфейса, который знает про FractalController: панель и холсты
 * сообщают о действиях пользователя событиями wxWidgets (и колбэками мыши),
 * а окно переводит их в вызовы контроллера и обновляет отображение.
 */
class MainWindow : public wxFrame {
public:
    MainWindow();

private:
    void buildMenu();
    void buildLayout();

    // обработчики событий (см. таблицу «действие -> обработчик» в пояснительной записке)
    void OnRenderClicked(wxCommandEvent&);
    void OnSaveClicked(wxCommandEvent&);
    void OnResetClicked(wxCommandEvent&);
    void OnResetView(wxCommandEvent&);
    void OnFractalChanged(wxCommandEvent&);
    void OnModeToggled(wxCommandEvent&);
    void OnAppearanceChanged(wxCommandEvent&);
    void OnZoomChanged(wxScrollEvent&);
    void OnHelp(wxCommandEvent&);
    void OnExit(wxCommandEvent&);
    void OnClose(wxCloseEvent&);
    void OnCharHook(wxKeyEvent&);

    // вызовы от холстов
    void OnCanvasZoom(int px, int py, double k);
    void OnCanvasPan(int dx, int dy);
    void OnCanvasResized();

    void renderNow();          ///< прочитать параметры панели, пересчитать и показать
    void recomputeAndShow();   ///< пересчитать при текущих параметрах (навигация)
    void showResult();         ///< показать готовую карту высот в нужном режиме
    void updateStatus();
    void update3DStatus();
    wxString overlayText() const;
    void switchPage(bool mode3D);

    fractal::FractalController controller_;
    ControlPanel* panel_ = nullptr;
    wxSimplebook* book_ = nullptr;
    Canvas2D* canvas2D_ = nullptr;
    Canvas3D* canvas3D_ = nullptr;
    wxString settingsPath_;
    bool busy_ = false;
    long lastMs_ = 0;
};

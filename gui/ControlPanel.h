// ControlPanel.h — правая панель: параметры фрактала, палитра, 3D, кнопки.
#pragma once
#include <wx/wx.h>

#include "FractalParams.h"

class wxSlider;
class wxSpinCtrl;
class wxColourPickerCtrl;
class PalettePreview;

/**
 * Панель собирается из wxStaticBoxSizer / wxFlexGridSizer (без абсолютных координат).
 * События элементов (wxEVT_CHOICE, wxEVT_SLIDER, wxEVT_BUTTON, ...) идут вверх по
 * иерархии окон и обрабатываются в MainWindow по идентификаторам из Ids.h.
 */
class ControlPanel : public wxPanel {
public:
    explicit ControlPanel(wxWindow* parent);

    /// Читает все параметры. При ошибке ввода возвращает false и пишет причину в error.
    bool readParams(fractal::FractalParams& p, wxString& error);
    /// Читает только параметры внешнего вида (палитра, 3D) — без проверки полей фрактала.
    void readAppearance(fractal::FractalParams& p) const;
    void applyParams(const fractal::FractalParams& p);

    /// Включает только те элементы, которые относятся к выбранному фракталу.
    void enableFor(fractal::FractalType t);
    fractal::FractalType selectedType() const;

    double zoomFactor() const;
    void setZoomFactor(double f);        ///< не порождает событий
    void setPalettePreview(int presetId, fractal::InterpMode m);

private:
    void OnSlider(wxCommandEvent& e);
    void updateLabels();
    static bool parseNumber(const wxString& text, double& value);

    wxChoice *choiceFractal_, *choicePalette_, *choiceInterp_, *choiceGrid_;
    wxSpinCtrl *spinIterations_, *spinSeed_;
    wxSlider *sliderZoom_, *sliderRough_, *sliderCycles_, *sliderHeight_;
    wxStaticText *labelZoom_, *labelRough_, *labelCycles_, *labelHeight_;
    wxTextCtrl *textJuliaRe_, *textJuliaIm_;
    wxColourPickerCtrl* pickerInside_;
    wxCheckBox* check3D_;
    wxRadioBox* radioMode_;
    PalettePreview* preview_;
    wxButton *btnRender_, *btnSave_, *btnReset_;

    friend class MainWindow;
};

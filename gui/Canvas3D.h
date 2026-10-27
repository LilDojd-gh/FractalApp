// Canvas3D.h — холст трёхмерного режима на основе wxGLCanvas (OpenGL 3.3 core).
#pragma once
#include <wx/glcanvas.h>
#include <wx/wx.h>

#include <functional>
#include <memory>

#include "FractalController.h"
#include "SurfaceRenderer.h"

class Canvas3D : public wxGLCanvas {
public:
    Canvas3D(wxWindow* parent, fractal::FractalController& controller);
    ~Canvas3D() override;

    /// Сетка в контроллере изменилась: её нужно заново загрузить в видеопамять.
    void meshChanged();
    void setWireframe(bool on) { wireframe_ = on; Refresh(false); }
    /// Снимок текущего кадра (для сохранения в файл).
    bool captureFrame(wxImage& out);

    std::function<void()> onCameraChanged;

private:
    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent&);
    void OnLeftDown(wxMouseEvent&);
    void OnLeftUp(wxMouseEvent&);
    void OnMotion(wxMouseEvent&);
    void OnWheel(wxMouseEvent&);
    void OnCaptureLost(wxMouseCaptureLostEvent&);
    bool prepareGL();
    void renderFrame(int pixelW, int pixelH);
    wxSize pixelSize() const;

    fractal::FractalController& controller_;
    std::unique_ptr<wxGLContext> context_;
    fractal::SurfaceRenderer renderer_;
    bool meshDirty_ = true;
    bool wireframe_ = false;
    bool glFailed_ = false;
    bool dragging_ = false;
    wxPoint last_;
};

#include "Canvas3D.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Util.h"

namespace {
const wxGLAttributes& glAttributes() {
    static wxGLAttributes attrs = [] {
        wxGLAttributes a;
        a.PlatformDefaults().RGBA().DoubleBuffer().Depth(24).EndList();
        return a;
    }();
    return attrs;
}
}  // namespace

Canvas3D::Canvas3D(wxWindow* parent, fractal::FractalController& controller)
    : wxGLCanvas(parent, glAttributes(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxGLCanvasName),
      controller_(controller) {
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetCursor(wxCursor(wxCURSOR_SIZING));
    Bind(wxEVT_PAINT, &Canvas3D::OnPaint, this);
    Bind(wxEVT_SIZE, &Canvas3D::OnSize, this);
    Bind(wxEVT_LEFT_DOWN, &Canvas3D::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &Canvas3D::OnLeftUp, this);
    Bind(wxEVT_MOTION, &Canvas3D::OnMotion, this);
    Bind(wxEVT_MOUSEWHEEL, &Canvas3D::OnWheel, this);
    Bind(wxEVT_MOUSE_CAPTURE_LOST, &Canvas3D::OnCaptureLost, this);
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});
}

Canvas3D::~Canvas3D() {
    if (context_ && renderer_.ready()) {
        SetCurrent(*context_);  // ресурсы GPU можно освободить только при активном контексте
        renderer_.release();
    }
}

wxSize Canvas3D::pixelSize() const {
    const wxSize cs = GetClientSize();
    const double sf = GetContentScaleFactor();  // для HiDPI-экранов
    return wxSize(std::max(1, static_cast<int>(std::lround(cs.x * sf))),
                  std::max(1, static_cast<int>(std::lround(cs.y * sf))));
}

bool Canvas3D::prepareGL() {
    if (glFailed_) return false;
    if (!context_) {
        wxGLContextAttrs attrs;
        attrs.PlatformDefaults().CoreProfile().OGLVersion(3, 3).EndList();
        context_ = std::make_unique<wxGLContext>(this, nullptr, &attrs);
        if (!context_->IsOK()) {
            glFailed_ = true;
            CallAfter([] {
                wxMessageBox(U8("Не удалось создать контекст OpenGL 3.3. Для 3D-режима нужен видеоадаптер "
                                "и драйвер с поддержкой OpenGL 3.3."),
                             U8("Ошибка OpenGL"), wxOK | wxICON_ERROR);
            });
            return false;
        }
    }
    SetCurrent(*context_);
    if (!renderer_.ready()) {
        std::string err;
        if (!renderer_.init(err)) {
            glFailed_ = true;
            CallAfter([err] { wxMessageBox(U8(err), U8("Ошибка OpenGL"), wxOK | wxICON_ERROR); });
            return false;
        }
        meshDirty_ = true;
    }
    return true;
}

void Canvas3D::meshChanged() {
    meshDirty_ = true;
    Refresh(false);
}

void Canvas3D::renderFrame(int w, int h) {
    if (meshDirty_) {
        renderer_.upload(controller_.mesh());
        meshDirty_ = false;
    }
    renderer_.beginFrame(w, h);
    renderer_.draw(controller_.camera(), static_cast<float>(w) / static_cast<float>(h), wireframe_);
}

void Canvas3D::OnPaint(wxPaintEvent&) {
    wxPaintDC dc(this);  // обязателен для корректной обработки wxEVT_PAINT
    if (!IsShownOnScreen() || !prepareGL()) return;
    const wxSize s = pixelSize();
    renderFrame(s.x, s.y);
    SwapBuffers();
}

void Canvas3D::OnSize(wxSizeEvent& e) {
    Refresh(false);
    e.Skip();
}

bool Canvas3D::captureFrame(wxImage& out) {
    if (!prepareGL()) return false;
    const wxSize s = pixelSize();
    renderFrame(s.x, s.y);  // рисуем в задний буфер и читаем его до SwapBuffers
    std::vector<unsigned char> rgb;
    if (!renderer_.readPixels(s.x, s.y, rgb)) return false;
    // OpenGL отдаёт строки снизу вверх — переворачиваем.
    auto* buf = static_cast<unsigned char*>(std::malloc(rgb.size()));
    const std::size_t rowBytes = static_cast<std::size_t>(s.x) * 3;
    for (int y = 0; y < s.y; ++y)
        std::memcpy(buf + static_cast<std::size_t>(y) * rowBytes,
                    rgb.data() + static_cast<std::size_t>(s.y - 1 - y) * rowBytes, rowBytes);
    out = wxImage(s.x, s.y, buf, false);
    return true;
}

void Canvas3D::OnLeftDown(wxMouseEvent& e) {
    SetFocus();
    dragging_ = true;
    last_ = e.GetPosition();
    CaptureMouse();
}

void Canvas3D::OnLeftUp(wxMouseEvent&) {
    dragging_ = false;
    if (HasCapture()) ReleaseMouse();
}

void Canvas3D::OnCaptureLost(wxMouseCaptureLostEvent&) { dragging_ = false; }

void Canvas3D::OnMotion(wxMouseEvent& e) {
    if (!dragging_) return;
    const wxPoint p = e.GetPosition();
    // Горизонталь — поворот вокруг вертикальной оси, вертикаль — угол места.
    controller_.camera().rotate(-(p.x - last_.x) * 0.4f, (p.y - last_.y) * 0.4f);
    last_ = p;
    Refresh(false);
    if (onCameraChanged) onCameraChanged();
}

void Canvas3D::OnWheel(wxMouseEvent& e) {
    const float steps = static_cast<float>(e.GetWheelRotation()) / static_cast<float>(std::max(1, e.GetWheelDelta()));
    controller_.camera().zoom(steps);
    Refresh(false);
    if (onCameraChanged) onCameraChanged();
}

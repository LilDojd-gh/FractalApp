#include "Canvas2D.h"

#include <wx/dcbuffer.h>

#include <algorithm>
#include <cmath>

#include "Util.h"

Canvas2D::Canvas2D(wxWindow* parent) : wxPanel(parent, wxID_ANY), resizeTimer_(this) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetCursor(wxCursor(wxCURSOR_CROSS));
    Bind(wxEVT_PAINT, &Canvas2D::OnPaint, this);
    Bind(wxEVT_SIZE, &Canvas2D::OnSize, this);
    Bind(wxEVT_MOUSEWHEEL, &Canvas2D::OnWheel, this);
    Bind(wxEVT_LEFT_DOWN, &Canvas2D::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &Canvas2D::OnLeftUp, this);
    Bind(wxEVT_MOTION, &Canvas2D::OnMotion, this);
    Bind(wxEVT_MOUSE_CAPTURE_LOST, &Canvas2D::OnCaptureLost, this);
    // Размер меняется потоком событий; пересчёт запускаем, когда пользователь закончил тянуть окно.
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        if (onResized) onResized();
    });
}

void Canvas2D::setImage(const fractal::Image2D& img, const wxString& overlay) {
    image_ = toWxImage(img);
    overlay_ = overlay;
    dragDelta_ = wxPoint(0, 0);
    rebuildShown();
    Refresh();
}

void Canvas2D::rebuildShown() {
    if (!image_.IsOk()) {
        shown_ = wxBitmap();
        return;
    }
    const wxSize cs = GetClientSize();
    if (cs.x <= 0 || cs.y <= 0) return;
    const int iw = image_.GetWidth(), ih = image_.GetHeight();
    if (iw == cs.x && ih == cs.y) {
        shown_ = wxBitmap(image_);
        origin_ = wxPoint(0, 0);
        oneToOne_ = true;
    } else {
        const double s = std::min(cs.x / static_cast<double>(iw), cs.y / static_cast<double>(ih));
        const int w = std::max(1, static_cast<int>(iw * s)), h = std::max(1, static_cast<int>(ih * s));
        shown_ = wxBitmap(image_.Scale(w, h, wxIMAGE_QUALITY_BILINEAR));
        origin_ = wxPoint((cs.x - w) / 2, (cs.y - h) / 2);
        oneToOne_ = false;
    }
}

void Canvas2D::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(wxColour(32, 35, 43)));
    dc.Clear();
    if (shown_.IsOk()) dc.DrawBitmap(shown_, origin_.x + dragDelta_.x, origin_.y + dragDelta_.y, false);
    if (!overlay_.IsEmpty()) {
        dc.SetFont(GetFont());
        const wxSize ts = dc.GetTextExtent(overlay_);
        dc.SetBrush(wxBrush(wxColour(0, 0, 0)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(8, 8, ts.x + 16, ts.y + 10);
        dc.SetTextForeground(*wxWHITE);
        dc.DrawText(overlay_, 16, 13);
    }
}

void Canvas2D::OnSize(wxSizeEvent& e) {
    rebuildShown();
    Refresh();
    resizeTimer_.StartOnce(150);
    e.Skip();
}

void Canvas2D::OnWheel(wxMouseEvent& e) {
    if (!oneToOne_ || !onZoom || dragging_) return;
    const double steps = static_cast<double>(e.GetWheelRotation()) / std::max(1, e.GetWheelDelta());
    const double k = std::pow(1.25, steps);
    onZoom(e.GetX(), e.GetY(), e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL ? k : 1.0);
}

void Canvas2D::OnLeftDown(wxMouseEvent& e) {
    SetFocus();
    if (!oneToOne_) return;  // панорамирование доступно только для алгебраических фракталов
    dragging_ = true;
    dragStart_ = e.GetPosition();
    dragDelta_ = wxPoint(0, 0);
    CaptureMouse();
    SetCursor(wxCursor(wxCURSOR_HAND));
}

void Canvas2D::OnMotion(wxMouseEvent& e) {
    if (!dragging_) return;
    dragDelta_ = e.GetPosition() - dragStart_;
    Refresh(false);
}

void Canvas2D::OnLeftUp(wxMouseEvent&) {
    if (!dragging_) return;
    dragging_ = false;
    if (HasCapture()) ReleaseMouse();
    SetCursor(wxCursor(wxCURSOR_CROSS));
    const wxPoint d = dragDelta_;
    dragDelta_ = wxPoint(0, 0);  // сбрасываем до пересчёта: setImage заменит картинку
    if ((d.x != 0 || d.y != 0) && onPan) onPan(d.x, d.y);
    Refresh(false);
}

void Canvas2D::OnCaptureLost(wxMouseCaptureLostEvent&) {
    dragging_ = false;
    dragDelta_ = wxPoint(0, 0);
    Refresh(false);
}

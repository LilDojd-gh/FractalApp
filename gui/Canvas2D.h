// Canvas2D.h — холст двумерного режима: вывод растра, масштабирование колесом и панорамирование мышью.
#pragma once
#include <wx/wx.h>

#include <functional>

#include "Image2D.h"

/**
 * Холст показывает готовое изображение. Перетаскивание мышью сначала сдвигает
 * уже построенную картинку (мгновенный отклик), а по отпусканию кнопки сообщает
 * о сдвиге через onPan — тогда главное окно пересчитывает кадр.
 * Для алгебраических фракталов изображение строится 1:1 по размеру холста;
 * карта Diamond-Square вписывается в окно с сохранением пропорций.
 */
class Canvas2D : public wxPanel {
public:
    explicit Canvas2D(wxWindow* parent);

    void setImage(const fractal::Image2D& img, const wxString& overlay);

    std::function<void(int px, int py, double k)> onZoom;  ///< колесо мыши: коэффициент масштаба
    std::function<void(int dx, int dy)> onPan;             ///< сдвиг в пикселях
    std::function<void()> onResized;                       ///< окно изменило размер (с задержкой)

private:
    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent&);
    void OnWheel(wxMouseEvent&);
    void OnLeftDown(wxMouseEvent&);
    void OnLeftUp(wxMouseEvent&);
    void OnMotion(wxMouseEvent&);
    void OnCaptureLost(wxMouseCaptureLostEvent&);
    void rebuildShown();

    wxImage image_;
    wxBitmap shown_;
    wxString overlay_;
    wxPoint origin_{0, 0};
    bool oneToOne_ = false;
    bool dragging_ = false;
    wxPoint dragStart_{0, 0}, dragDelta_{0, 0};
    wxTimer resizeTimer_;
};

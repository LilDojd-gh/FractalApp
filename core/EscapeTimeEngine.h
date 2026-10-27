// EscapeTimeEngine.h — общий алгоритм «времени побега» для множеств Мандельброта и Жюлиа.
#pragma once
#include "FractalEngine.h"

namespace fractal {

/**
 * Шаблонный метод: compute() обходит пиксели (параллельно по строкам) и для каждого
 * вызывает виртуальный setup(), который наследник использует, чтобы задать z0 и c.
 * Результат — дробное число итераций nu (гладкая раскраска), нормированное на N_max.
 */
class EscapeTimeEngine : public FractalEngine {
public:
    /// Радиус побега для гладкой раскраски (R = 256 уменьшает погрешность формулы nu).
    static constexpr double kBailout = 256.0;

    EscapeTimeEngine(double centerX, double centerY, double width);

    void compute(HeightMap& out) override;

    bool supportsNavigation() const override { return true; }
    void resetView() override;
    void zoomAt(int px, int py, double k, int w, int h) override;
    void pan(int dx, int dy, int w) override;
    double zoomFactor() const override { return defaultWidth_ / width_; }
    void setZoomFactor(double f) override;
    std::string describeView() const override;

    /// Точка комплексной плоскости, соответствующая центру пикселя (px, py).
    Complex pixelToComplex(int px, int py, int w, int h) const;
    Complex center() const { return {centerX_, centerY_}; }
    double viewWidth() const { return width_; }

    /// Дробное число итераций nu для орбиты z -> z^2 + c; -1, если орбита не ушла за maxIter шагов.
    static double iterate(Complex z0, Complex c, int maxIter);

protected:
    /// Задаёт начальные значения орбиты для точки плоскости p.
    virtual void setup(const Complex& p, Complex& z0, Complex& c) const = 0;
    /// Быстрая проверка принадлежности множеству без итераций (по умолчанию — нет).
    virtual bool quickInside(const Complex& /*p*/) const { return false; }

private:
    double defaultCenterX_, defaultCenterY_, defaultWidth_;
    double centerX_, centerY_, width_;  // width_ — ширина окна просмотра по оси Re
};

}  // namespace fractal

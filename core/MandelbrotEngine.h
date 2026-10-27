// MandelbrotEngine.h — множество Мандельброта: z0 = 0, c = точка плоскости.
#pragma once
#include "EscapeTimeEngine.h"

namespace fractal {

class MandelbrotEngine : public EscapeTimeEngine {
public:
    MandelbrotEngine() : EscapeTimeEngine(-0.5, 0.0, 3.5) {}
    FractalType type() const override { return FractalType::Mandelbrot; }

protected:
    void setup(const Complex& p, Complex& z0, Complex& c) const override {
        z0 = Complex(0.0, 0.0);
        c = p;
    }
    /// Главная кардиоида и круговая луковица периода 2 заведомо принадлежат множеству.
    bool quickInside(const Complex& p) const override {
        const double x = p.real(), y = p.imag();
        const double xq = x - 0.25;
        const double q = xq * xq + y * y;
        if (q * (q + xq) < 0.25 * y * y) return true;
        const double xb = x + 1.0;
        return xb * xb + y * y < 0.0625;
    }
};

}  // namespace fractal

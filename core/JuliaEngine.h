// JuliaEngine.h — множество Жюлиа: z0 = точка плоскости, c = константа.
#pragma once
#include "EscapeTimeEngine.h"

namespace fractal {

class JuliaEngine : public EscapeTimeEngine {
public:
    JuliaEngine() : EscapeTimeEngine(0.0, 0.0, 3.2) {}
    FractalType type() const override { return FractalType::Julia; }

    void setConstant(const Complex& c) { constC_ = c; }
    Complex constant() const { return constC_; }

    void applyParams(const FractalParams& p) override {
        EscapeTimeEngine::applyParams(p);
        constC_ = Complex(p.juliaRe, p.juliaIm);
    }

protected:
    void setup(const Complex& p, Complex& z0, Complex& c) const override {
        z0 = p;
        c = constC_;
    }

private:
    Complex constC_{-0.8, 0.156};
};

}  // namespace fractal

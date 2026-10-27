// EngineFactory.h — фабрика генераторов: скрывает конкретные классы от контроллера.
#pragma once
#include <memory>

#include "DiamondSquareEngine.h"
#include "JuliaEngine.h"
#include "MandelbrotEngine.h"

namespace fractal {

class EngineFactory {
public:
    static std::unique_ptr<FractalEngine> create(FractalType t) {
        switch (t) {
            case FractalType::Julia: return std::make_unique<JuliaEngine>();
            case FractalType::DiamondSquare: return std::make_unique<DiamondSquareEngine>();
            case FractalType::Mandelbrot:
            default: return std::make_unique<MandelbrotEngine>();
        }
    }
};

}  // namespace fractal

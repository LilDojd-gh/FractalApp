// DiamondSquareEngine.h — стохастическая фрактальная поверхность (алгоритм смещения середин).
#pragma once
#include <cstdint>

#include "FractalEngine.h"

namespace fractal {

class DiamondSquareEngine : public FractalEngine {
public:
    FractalType type() const override { return FractalType::DiamondSquare; }

    /// Строит карту (2^n + 1) x (2^n + 1) и нормирует её в [0, 1].
    void compute(HeightMap& out) override;

    void applyParams(const FractalParams& p) override;
    void setRoughness(float h) { roughness_ = std::clamp(h, 0.05f, 1.0f); }
    void setSeed(std::uint32_t s) { seed_ = s; }
    void setGridLevel(int n) { gridLevel_ = std::clamp(n, 1, 12); }
    float roughness() const { return roughness_; }
    int gridLevel() const { return gridLevel_; }

private:
    float roughness_ = 0.8f;  // параметр Херста H
    std::uint32_t seed_ = 42;
    int gridLevel_ = 7;
};

}  // namespace fractal

#include "DiamondSquareEngine.h"

#include <cmath>
#include <random>

namespace fractal {

void DiamondSquareEngine::applyParams(const FractalParams& p) {
    FractalEngine::applyParams(p);
    setRoughness(p.roughness);
    setSeed(p.seed);
    setGridLevel(p.gridLevel);
}

void DiamondSquareEngine::compute(HeightMap& out) {
    const int N = (1 << gridLevel_) + 1;
    out.resize(N, N);

    // mt19937 — детерминирован стандартом; переводим в [0, 1) вручную, чтобы результат
    // не зависел от реализации std::uniform_real_distribution в разных компиляторах.
    std::mt19937 rng(seed_);
    auto rnd01 = [&rng] { return static_cast<float>(rng() >> 8) * (1.0f / 16777216.0f); };
    auto rndSigned = [&](float s) { return (rnd01() * 2.0f - 1.0f) * s; };

    out.set(0, 0, rnd01());
    out.set(N - 1, 0, rnd01());
    out.set(0, N - 1, rnd01());
    out.set(N - 1, N - 1, rnd01());

    float sigma = 1.0f;
    const float decay = std::pow(2.0f, -roughness_);  // sigma_{k+1} = sigma_k * 2^{-H}
    for (int size = N - 1; size > 1; size /= 2) {
        const int half = size / 2;

        // Шаг Diamond: центр каждого квадрата = среднее четырёх углов + смещение.
        for (int y = half; y < N; y += size)
            for (int x = half; x < N; x += size) {
                const float avg = (out.at(x - half, y - half) + out.at(x + half, y - half) +
                                   out.at(x - half, y + half) + out.at(x + half, y + half)) * 0.25f;
                out.set(x, y, avg + rndSigned(sigma));
            }

        // Шаг Square: центр каждого ромба = среднее существующих соседей + смещение.
        for (int y = 0; y < N; y += half)
            for (int x = ((y / half) % 2 == 0) ? half : 0; x < N; x += size) {
                float sum = 0.0f;
                int n = 0;
                if (y - half >= 0) { sum += out.at(x, y - half); ++n; }
                if (y + half < N)  { sum += out.at(x, y + half); ++n; }
                if (x - half >= 0) { sum += out.at(x - half, y); ++n; }
                if (x + half < N)  { sum += out.at(x + half, y); ++n; }
                out.set(x, y, sum / static_cast<float>(n) + rndSigned(sigma));
            }

        sigma *= decay;
    }
    out.normalize();
}

}  // namespace fractal

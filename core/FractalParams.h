// FractalParams.h — параметры генерации и отображения (формируются панелью управления).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "ColorPalette.h"
#include "Types.h"

namespace fractal {

/// Структура-значение: все параметры, которые пользователь меняет в интерфейсе.
struct FractalParams {
    // --- генерация ---
    FractalType type = FractalType::Mandelbrot;
    int maxIterations = 500;       ///< N_max, 10..5000
    double juliaRe = -0.8;         ///< Re(c) для множества Жюлиа
    double juliaIm = 0.156;        ///< Im(c) для множества Жюлиа
    float roughness = 0.8f;        ///< параметр Херста H для Diamond-Square, 0.05..1
    std::uint32_t seed = 42;       ///< начальное значение датчика Diamond-Square
    int gridLevel = 7;             ///< размер карты Diamond-Square: 2^n + 1, n = 6..10
    // --- цвет ---
    int presetId = 1;              ///< номер встроенной палитры
    InterpMode interp = InterpMode::Linear;
    Rgb insideColor{0, 0, 0};      ///< цвет точек, принадлежащих множеству
    float paletteCycles = 30.0f;   ///< S — число повторов палитры (только для Escape-Time)
    // --- 3D ---
    bool mode3D = false;
    bool wireframe = false;
    float heightScale = 0.5f;      ///< S_z

    /// Приводит значения к допустимым диапазонам (в том числе после чтения файла).
    void clamp() {
        auto fin = [](double v, double def) { return std::isfinite(v) ? v : def; };
        maxIterations = std::clamp(maxIterations, 10, 5000);
        juliaRe = std::clamp(fin(juliaRe, -0.8), -2.5, 2.5);
        juliaIm = std::clamp(fin(juliaIm, 0.156), -2.5, 2.5);
        roughness = std::clamp(static_cast<float>(fin(roughness, 0.8)), 0.05f, 1.0f);
        gridLevel = std::clamp(gridLevel, 6, 10);
        presetId = std::clamp(presetId, 0, kPresetCount - 1);
        paletteCycles = std::clamp(static_cast<float>(fin(paletteCycles, 30.0)), 1.0f, 100.0f);
        heightScale = std::clamp(static_cast<float>(fin(heightScale, 0.5)), 0.1f, 1.5f);
    }

    /// Палитра, которая лучше всего подходит выбранному фракталу.
    static int defaultPresetFor(FractalType t) { return t == FractalType::DiamondSquare ? 4 : 1; }
};

}  // namespace fractal

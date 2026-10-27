// Types.h — базовые типы вычислительного ядра (не зависят от wxWidgets и OpenGL).
#pragma once
#include <complex>
#include <cstdint>

namespace fractal {

using Complex = std::complex<double>;

/// Цвет в формате RGB, 8 бит на канал.
struct Rgb {
    std::uint8_t r = 0, g = 0, b = 0;
};

/// Тип генератора фрактала.
enum class FractalType { Mandelbrot = 0, Julia = 1, DiamondSquare = 2 };

/// Способ интерполяции цвета в палитре.
enum class InterpMode { Linear = 0, Logarithmic = 1 };

}  // namespace fractal

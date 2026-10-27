// ColorPalette.h — палитра: узлы цвета, интерполяция, цвет «внутри множества».
#pragma once
#include <string>
#include <vector>

#include "Types.h"

namespace fractal {

/// Количество встроенных палитр.
constexpr int kPresetCount = 5;

struct ColorStop {
    float pos;  ///< положение узла в [0, 1]
    Rgb color;
};

/**
 * Палитра — функция Pal: [0, 1] -> RGB, заданная упорядоченным набором узлов.
 * Между узлами цвет интерполируется линейно; в режиме Logarithmic аргумент
 * предварительно искажается: t' = ln(1 + beta*t) / ln(1 + beta).
 */
class ColorPalette {
public:
    ColorPalette();

    static int presetCount() { return kPresetCount; }
    /// Название встроенной палитры (UTF-8).
    static const char* presetName(int id);

    void loadPreset(int id);
    int presetId() const { return presetId_; }

    void clearStops();
    /// Добавляет узел, сохраняя упорядоченность по pos.
    void addStop(float pos, Rgb color);
    const std::vector<ColorStop>& stops() const { return stops_; }

    void setInterpolation(InterpMode m) { mode_ = m; }
    InterpMode interpolation() const { return mode_; }
    void setInsideColor(Rgb c) { inside_ = c; }
    Rgb insideColor() const { return inside_; }

    /// Цвет для t; значения вне [0, 1] обрезаются.
    Rgb colorAt(float t) const;

private:
    std::vector<ColorStop> stops_;
    InterpMode mode_ = InterpMode::Linear;
    Rgb inside_{0, 0, 0};
    int presetId_ = 0;
};

}  // namespace fractal

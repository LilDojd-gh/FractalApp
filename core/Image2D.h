// Image2D.h — RGB-буфер растрового изображения (не зависит от wxWidgets).
#pragma once
#include <cstdint>
#include <vector>

#include "Types.h"

namespace fractal {

class Image2D {
public:
    Image2D(int width = 0, int height = 0) { resize(width, height); }

    void resize(int width, int height) {
        width_ = width > 0 ? width : 0;
        height_ = height > 0 ? height : 0;
        pixels_.assign(static_cast<std::size_t>(width_) * height_ * 3, 0);
    }
    int width() const { return width_; }
    int height() const { return height_; }

    void setPixel(int x, int y, Rgb c) {
        std::uint8_t* p = &pixels_[(static_cast<std::size_t>(y) * width_ + x) * 3];
        p[0] = c.r; p[1] = c.g; p[2] = c.b;
    }
    Rgb pixel(int x, int y) const {
        const std::uint8_t* p = &pixels_[(static_cast<std::size_t>(y) * width_ + x) * 3];
        return Rgb{p[0], p[1], p[2]};
    }
    /// Данные RGB подряд, по 3 байта на пиксель, строки сверху вниз.
    const std::uint8_t* data() const { return pixels_.data(); }
    std::size_t byteSize() const { return pixels_.size(); }

private:
    int width_ = 0, height_ = 0;
    std::vector<std::uint8_t> pixels_;
};

}  // namespace fractal

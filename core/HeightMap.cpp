#include "HeightMap.h"

#include <algorithm>
#include <glm/geometric.hpp>

namespace fractal {

HeightMap::HeightMap(int width, int height) { resize(width, height); }

void HeightMap::resize(int width, int height) {
    width_ = std::max(0, width);
    height_ = std::max(0, height);
    const std::size_t n = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    data_.assign(n, 0.0f);
    inside_.assign(n, 0);
}

void HeightMap::normalize() {
    if (data_.empty()) return;
    const auto mm = std::minmax_element(data_.begin(), data_.end());
    const float lo = *mm.first, hi = *mm.second;
    if (hi - lo < 1e-12f) {
        std::fill(data_.begin(), data_.end(), 0.0f);
        return;
    }
    const float inv = 1.0f / (hi - lo);
    for (float& v : data_) v = (v - lo) * inv;
}

bool HeightMap::minMaxOutside(float& lo, float& hi) const {
    bool any = false;
    for (std::size_t i = 0; i < data_.size(); ++i) {
        if (inside_[i]) continue;
        if (!any) {
            lo = hi = data_[i];
            any = true;
        } else {
            lo = std::min(lo, data_[i]);
            hi = std::max(hi, data_[i]);
        }
    }
    return any;
}

glm::vec3 HeightMap::normalAt(int x, int y, float cellSize, float heightScale) const {
    const int xl = std::max(x - 1, 0), xr = std::min(x + 1, width_ - 1);
    const int yl = std::max(y - 1, 0), yr = std::min(y + 1, height_ - 1);
    const float dx = static_cast<float>(std::max(1, xr - xl)) * cellSize;
    const float dz = static_cast<float>(std::max(1, yr - yl)) * cellSize;
    const float dhdx = (at(xr, y) - at(xl, y)) / dx;
    const float dhdz = (at(x, yr) - at(x, yl)) / dz;
    return glm::normalize(glm::vec3(-heightScale * dhdx, 1.0f, -heightScale * dhdz));
}

HeightMap HeightMap::downsampled(int maxSide) const {
    if (empty()) return HeightMap();
    maxSide = std::max(2, maxSide);
    const int longest = std::max(width_, height_);
    const int step = std::max(1, (longest - 1 + maxSide - 2) / (maxSide - 1));
    HeightMap r((width_ - 1) / step + 1, (height_ - 1) / step + 1);
    for (int j = 0; j < r.height(); ++j)
        for (int i = 0; i < r.width(); ++i) {
            r.set(i, j, at(i * step, j * step));
            r.setInside(i, j, isInside(i * step, j * step));
        }
    return r;
}

}  // namespace fractal

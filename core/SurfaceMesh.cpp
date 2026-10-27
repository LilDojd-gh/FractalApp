#include "SurfaceMesh.h"

#include <algorithm>

namespace fractal {

void SurfaceMesh::build(const HeightMap& src, float heightScale, const ColorFn& colorFn, int maxGrid) {
    clear();
    cols_ = rows_ = 0;
    if (src.width() < 2 || src.height() < 2) return;

    const HeightMap g = src.downsampled(maxGrid);
    cols_ = g.width();
    rows_ = g.height();

    // Рельеф: нормируем высоты по точкам вне множества, точки внутри ставим на нулевой уровень.
    float lo = 0, hi = 0;
    const bool any = g.minMaxOutside(lo, hi);
    const bool flat = !any || hi - lo < 1e-9f;
    HeightMap relief(cols_, rows_);
    for (int j = 0; j < rows_; ++j)
        for (int i = 0; i < cols_; ++i)
            relief.set(i, j, (flat || g.isInside(i, j)) ? 0.0f : (g.at(i, j) - lo) / (hi - lo));

    const float cell = 2.0f / static_cast<float>(std::max(cols_, rows_) - 1);
    vertices_.reserve(static_cast<std::size_t>(cols_) * rows_);
    for (int j = 0; j < rows_; ++j)
        for (int i = 0; i < cols_; ++i) {
            Vertex v;
            v.position = glm::vec3((static_cast<float>(i) - (cols_ - 1) * 0.5f) * cell,
                                   relief.at(i, j) * heightScale,
                                   (static_cast<float>(j) - (rows_ - 1) * 0.5f) * cell);
            v.normal = relief.normalAt(i, j, cell, heightScale);
            const Rgb c = colorFn(g.at(i, j), g.isInside(i, j));
            v.color = glm::vec3(c.r, c.g, c.b) / 255.0f;
            vertices_.push_back(v);
        }

    indices_.reserve(static_cast<std::size_t>(cols_ - 1) * (rows_ - 1) * 6);
    for (int j = 0; j + 1 < rows_; ++j)
        for (int i = 0; i + 1 < cols_; ++i) {
            const std::uint32_t a = static_cast<std::uint32_t>(j * cols_ + i);  // V(i, j)
            const std::uint32_t b = a + static_cast<std::uint32_t>(cols_);      // V(i, j+1)
            const std::uint32_t c = a + 1;                                       // V(i+1, j)
            const std::uint32_t d = b + 1;                                       // V(i+1, j+1)
            indices_.insert(indices_.end(), {a, b, c, c, b, d});
        }
}

}  // namespace fractal

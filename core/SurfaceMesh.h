// SurfaceMesh.h — треугольная сетка поверхности, построенная по карте высот.
#pragma once
#include <cstdint>
#include <functional>
#include <glm/vec3.hpp>
#include <vector>

#include "HeightMap.h"
#include "Types.h"

namespace fractal {

/// Вершина: позиция, нормаль и цвет (компоненты цвета в [0, 1]). Размер — 9 float подряд.
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

/**
 * Регулярная сетка: узел (i, j) -> V = (i*dx, S_z*H, j*dz), каждая ячейка — два
 * треугольника с обходом против часовой стрелки при взгляде сверху.
 * Сцена нормирована: большая сторона сетки имеет длину 2 и центрирована в нуле.
 */
class SurfaceMesh {
public:
    /// Функция раскраски: (исходная высота, признак «внутри множества») -> цвет.
    using ColorFn = std::function<Rgb(float, bool)>;

    /// Строит сетку. Карта предварительно прореживается до maxGrid узлов по большей стороне.
    /// Для рельефа высоты нормируются по точкам вне множества; точки внутри имеют H = 0.
    void build(const HeightMap& src, float heightScale, const ColorFn& colorFn, int maxGrid = 257);

    void clear() { vertices_.clear(); indices_.clear(); }
    bool empty() const { return indices_.empty(); }
    const std::vector<Vertex>& vertices() const { return vertices_; }
    const std::vector<std::uint32_t>& indices() const { return indices_; }
    int columns() const { return cols_; }
    int rows() const { return rows_; }

private:
    std::vector<Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
    int cols_ = 0, rows_ = 0;
};

}  // namespace fractal

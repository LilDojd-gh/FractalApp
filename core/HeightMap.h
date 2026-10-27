// HeightMap.h — карта высот: нормированные значения H(x, y) и маска «точка внутри множества».
#pragma once
#include <glm/vec3.hpp>
#include <vector>

namespace fractal {

/**
 * Контейнер карты высот. Значения хранятся в непрерывном массиве (строка за строкой).
 * Для алгебраических фракталов дополнительно хранится маска точек, принадлежащих
 * множеству (у них нет дробного числа итераций). Разные строки можно безопасно
 * записывать из разных потоков.
 */
class HeightMap {
public:
    explicit HeightMap(int width = 0, int height = 0);

    /// Меняет размер; все значения обнуляются, маска очищается.
    void resize(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }
    bool empty() const { return width_ == 0 || height_ == 0; }

    float at(int x, int y) const { return data_[index(x, y)]; }
    void set(int x, int y, float value) { data_[index(x, y)] = value; }
    bool isInside(int x, int y) const { return inside_[index(x, y)] != 0; }
    void setInside(int x, int y, bool value) { inside_[index(x, y)] = value ? 1 : 0; }

    /// Линейная нормировка всех значений в [0, 1]. Для постоянной карты — нули.
    void normalize();

    /// Минимум и максимум среди точек вне множества. false, если таких точек нет.
    bool minMaxOutside(float& lo, float& hi) const;

    /// Нормаль в точке по конечным разностям (центральные; на границе — односторонние).
    /// cellSize — расстояние между соседними узлами, heightScale — масштаб высоты S_z.
    glm::vec3 normalAt(int x, int y, float cellSize, float heightScale) const;

    /// Уменьшенная копия, у которой каждая сторона не больше maxSide узлов
    /// (берётся каждый step-й узел). Маска переносится.
    HeightMap downsampled(int maxSide) const;

private:
    int index(int x, int y) const { return y * width_ + x; }

    int width_ = 0;
    int height_ = 0;
    std::vector<float> data_;
    std::vector<unsigned char> inside_;
};

}  // namespace fractal

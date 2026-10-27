// FractalEngine.h — абстрактный генератор фрактала: заполняет карту высот.
#pragma once
#include <string>

#include "FractalParams.h"
#include "HeightMap.h"

namespace fractal {

/**
 * Базовый класс иерархии генераторов (полиморфный интерфейс).
 * Контроллер работает только с этим интерфейсом, поэтому тип фрактала можно
 * менять во время работы. Методы навигации по умолчанию ничего не делают —
 * их переопределяют генераторы, у которых есть окно просмотра.
 */
class FractalEngine {
public:
    virtual ~FractalEngine() = default;

    /// Заполняет out (размер задаёт вызывающий; Diamond-Square выбирает размер сам).
    virtual void compute(HeightMap& out) = 0;
    virtual FractalType type() const = 0;

    /// Переносит параметры из структуры в генератор.
    virtual void applyParams(const FractalParams& p) { setMaxIterations(p.maxIterations); }

    void setMaxIterations(int n) { maxIterations_ = std::clamp(n, 1, 100000); }
    int maxIterations() const { return maxIterations_; }

    // --- навигация по окну просмотра (только для алгебраических фракталов) ---
    virtual bool supportsNavigation() const { return false; }
    virtual void resetView() {}
    /// Масштабирование в k раз относительно пикселя (px, py) изображения w x h.
    virtual void zoomAt(int /*px*/, int /*py*/, double /*k*/, int /*w*/, int /*h*/) {}
    /// Сдвиг изображения на (dx, dy) пикселей вслед за мышью; w — ширина изображения.
    virtual void pan(int /*dx*/, int /*dy*/, int /*w*/) {}
    virtual double zoomFactor() const { return 1.0; }
    virtual void setZoomFactor(double /*f*/) {}
    virtual std::string describeView() const { return {}; }

protected:
    int maxIterations_ = 500;
};

}  // namespace fractal

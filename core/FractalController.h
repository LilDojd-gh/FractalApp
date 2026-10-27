// FractalController.h — единая точка входа ядра для интерфейса.
#pragma once
#include <memory>
#include <string>

#include "Camera3D.h"
#include "ColorPalette.h"
#include "FractalEngine.h"
#include "FractalParams.h"
#include "HeightMap.h"
#include "Image2D.h"
#include "SurfaceMesh.h"

namespace fractal {

/**
 * Владеет генератором (std::unique_ptr<FractalEngine>), картой высот, палитрой,
 * камерой и сеткой. Интерфейс общается с ядром только через этот класс.
 * Расчёт (recompute) отделён от раскраски (render2D / buildMesh): смена палитры
 * или масштаба высоты не требует повторного расчёта фрактала.
 */
class FractalController {
public:
    FractalController();

    /// Применяет все параметры; при смене типа создаётся новый генератор.
    void setParams(const FractalParams& p);
    /// Применяет только параметры внешнего вида (палитра, цикл, 3D) — без пересоздания генератора.
    void updateAppearance(const FractalParams& p);
    const FractalParams& params() const { return params_; }

    /// Размер растра для алгебраических фракталов (Diamond-Square выбирает размер сам).
    void setResolution(int w, int h);
    void recompute();
    bool hasData() const { return computed_; }

    /// Цвет точки карты: Escape-Time — Pal(fract(S*h)), Diamond-Square — Pal(h).
    Rgb colorOf(float h, bool inside) const;
    Image2D render2D() const;
    void buildMesh();

    // --- навигация по окну просмотра (для Escape-Time) ---
    bool supportsNavigation() const { return engine_ && engine_->supportsNavigation(); }
    void zoomAt(int px, int py, double k);
    void pan(int dx, int dy);
    void resetView();
    void setZoomFactor(double f);
    double zoomFactor() const { return engine_ ? engine_->zoomFactor() : 1.0; }
    std::string describeView() const { return engine_ ? engine_->describeView() : std::string(); }

    Camera3D& camera() { return camera_; }
    const Camera3D& camera() const { return camera_; }
    const SurfaceMesh& mesh() const { return mesh_; }
    const HeightMap& heightMap() const { return heightMap_; }
    const ColorPalette& palette() const { return palette_; }

private:
    void applyAppearance();

    FractalParams params_;
    std::unique_ptr<FractalEngine> engine_;
    HeightMap heightMap_;
    ColorPalette palette_;
    Camera3D camera_;
    SurfaceMesh mesh_;
    int width_ = 800, height_ = 600;
    bool computed_ = false;
};

}  // namespace fractal

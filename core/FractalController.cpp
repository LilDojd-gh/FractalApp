#include "FractalController.h"

#include <cmath>

#include "EngineFactory.h"

namespace fractal {

FractalController::FractalController() { setParams(FractalParams{}); }

void FractalController::setParams(const FractalParams& p) {
    params_ = p;
    params_.clamp();
    if (!engine_ || engine_->type() != params_.type) {
        engine_ = EngineFactory::create(params_.type);
        computed_ = false;
    }
    engine_->applyParams(params_);
    applyAppearance();
}

void FractalController::updateAppearance(const FractalParams& p) {
    FractalParams q = p;
    q.clamp();
    params_.presetId = q.presetId;
    params_.interp = q.interp;
    params_.insideColor = q.insideColor;
    params_.paletteCycles = q.paletteCycles;
    params_.mode3D = q.mode3D;
    params_.wireframe = q.wireframe;
    params_.heightScale = q.heightScale;
    applyAppearance();
}

void FractalController::applyAppearance() {
    palette_.loadPreset(params_.presetId);
    palette_.setInterpolation(params_.interp);
    palette_.setInsideColor(params_.insideColor);
}

void FractalController::setResolution(int w, int h) {
    width_ = std::max(16, w);
    height_ = std::max(16, h);
}

void FractalController::recompute() {
    if (!engine_) return;
    // Escape-Time рисует в растр заданного размера; Diamond-Square сам меняет размер карты.
    if (heightMap_.width() != width_ || heightMap_.height() != height_ ||
        engine_->type() != FractalType::DiamondSquare)
        heightMap_.resize(width_, height_);
    engine_->compute(heightMap_);
    computed_ = true;
}

Rgb FractalController::colorOf(float h, bool inside) const {
    if (inside) return palette_.insideColor();
    if (params_.type == FractalType::DiamondSquare) return palette_.colorAt(h);
    const float s = params_.paletteCycles * h;
    return palette_.colorAt(s - std::floor(s));  // t = fract(S * nu / N_max)
}

Image2D FractalController::render2D() const {
    Image2D img(heightMap_.width(), heightMap_.height());
    for (int y = 0; y < heightMap_.height(); ++y)
        for (int x = 0; x < heightMap_.width(); ++x)
            img.setPixel(x, y, colorOf(heightMap_.at(x, y), heightMap_.isInside(x, y)));
    return img;
}

void FractalController::buildMesh() {
    mesh_.build(heightMap_, params_.heightScale,
                [this](float h, bool inside) { return colorOf(h, inside); });
}

void FractalController::zoomAt(int px, int py, double k) {
    if (engine_ && computed_) engine_->zoomAt(px, py, k, heightMap_.width(), heightMap_.height());
}

void FractalController::pan(int dx, int dy) {
    if (engine_ && computed_) engine_->pan(dx, dy, heightMap_.width());
}

void FractalController::resetView() {
    if (engine_) engine_->resetView();
    camera_.reset();
}

void FractalController::setZoomFactor(double f) {
    if (engine_) engine_->setZoomFactor(f);
}

}  // namespace fractal

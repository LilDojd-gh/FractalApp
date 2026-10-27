#include "EscapeTimeEngine.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <sstream>
#include <thread>
#include <vector>

namespace fractal {

EscapeTimeEngine::EscapeTimeEngine(double cx, double cy, double width)
    : defaultCenterX_(cx), defaultCenterY_(cy), defaultWidth_(width),
      centerX_(cx), centerY_(cy), width_(width) {}

void EscapeTimeEngine::resetView() {
    centerX_ = defaultCenterX_;
    centerY_ = defaultCenterY_;
    width_ = defaultWidth_;
}

Complex EscapeTimeEngine::pixelToComplex(int px, int py, int w, int h) const {
    const double height = width_ * static_cast<double>(h) / static_cast<double>(w);
    return {centerX_ + ((px + 0.5) / w - 0.5) * width_, centerY_ - ((py + 0.5) / h - 0.5) * height};
}

void EscapeTimeEngine::zoomAt(int px, int py, double k, int w, int h) {
    if (w <= 0 || h <= 0 || !(k > 0.0)) return;
    const Complex p = pixelToComplex(px, py, w, h);
    // Ограничиваем итоговый коэффициент диапазоном [0.25, 1e13] (предел точности double).
    const double newWidth = std::clamp(width_ / k, defaultWidth_ * 1e-13, defaultWidth_ * 4.0);
    const double keff = width_ / newWidth;
    // Точка p остаётся на месте: новый центр = p - (p - центр) / keff.
    centerX_ = p.real() - (p.real() - centerX_) / keff;
    centerY_ = p.imag() - (p.imag() - centerY_) / keff;
    width_ = newWidth;
}

void EscapeTimeEngine::pan(int dx, int dy, int w) {
    if (w <= 0) return;
    const double pixel = width_ / w;
    centerX_ -= dx * pixel;  // изображение уезжает вслед за мышью
    centerY_ += dy * pixel;  // ось пикселей Y направлена вниз, ось Im — вверх
}

void EscapeTimeEngine::setZoomFactor(double f) {
    f = std::clamp(f, 0.25, 1e13);
    width_ = defaultWidth_ / f;
}

std::string EscapeTimeEngine::describeView() const {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os.precision(12);
    os << "Центр: " << centerX_ << (centerY_ < 0 ? " - " : " + ") << std::abs(centerY_)
       << "i    Масштаб: x";
    os.precision(6);
    os << zoomFactor() << "    Итераций: " << maxIterations_;
    return os.str();
}

double EscapeTimeEngine::iterate(Complex z0, Complex c, int maxIter) {
    double zr = z0.real(), zi = z0.imag();
    const double cr = c.real(), ci = c.imag();
    const double bail2 = kBailout * kBailout;
    for (int k = 0; k < maxIter; ++k) {
        const double zr2 = zr * zr, zi2 = zi * zi;
        const double m2 = zr2 + zi2;
        if (m2 > bail2) {
            // nu = k + 1 - log2(ln|z|),  ln|z| = 0.5 * ln|z|^2
            const double nu = k + 1 - std::log2(0.5 * std::log(m2));
            return nu > 0.0 ? nu : 0.0;
        }
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
    }
    return -1.0;
}

void EscapeTimeEngine::compute(HeightMap& out) {
    const int W = out.width(), H = out.height();
    if (W <= 0 || H <= 0) return;
    const int maxIter = maxIterations_;

    auto renderRow = [&](int j) {
        for (int i = 0; i < W; ++i) {
            const Complex p = pixelToComplex(i, j, W, H);
            double nu = -1.0;
            if (!quickInside(p)) {
                Complex z0, c;
                setup(p, z0, c);
                nu = iterate(z0, c, maxIter);
            }
            if (nu < 0.0) {
                out.set(i, j, 0.0f);
                out.setInside(i, j, true);
            } else {
                out.set(i, j, static_cast<float>(std::min(1.0, nu / maxIter)));
                out.setInside(i, j, false);
            }
        }
    };

    // Строки раздаются потокам через атомарный счётчик (естественная балансировка нагрузки).
    std::atomic<int> nextRow{0};
    auto worker = [&] {
        for (int j = nextRow.fetch_add(1); j < H; j = nextRow.fetch_add(1)) renderRow(j);
    };
    const int threads = std::clamp(static_cast<int>(std::thread::hardware_concurrency()), 1, H);
    std::vector<std::thread> pool;
    for (int t = 1; t < threads; ++t) pool.emplace_back(worker);
    worker();
    for (auto& t : pool) t.join();
}

}  // namespace fractal

#include "ColorPalette.h"

#include <algorithm>
#include <cmath>

namespace fractal {

namespace {
constexpr float kLogBeta = 9.0f;

struct Preset {
    const char* name;
    std::vector<ColorStop> stops;
};

const std::vector<Preset>& presets() {
    static const std::vector<Preset> p = {
        {"Классический",  // палитра замкнута (первый и последний цвета совпадают)
         {{0.0f, {0, 7, 100}}, {0.16f, {32, 107, 203}}, {0.42f, {237, 255, 255}},
          {0.6425f, {255, 170, 0}}, {0.8575f, {0, 2, 0}}, {1.0f, {0, 7, 100}}}},
        {"Огненный",
         {{0.0f, {0, 0, 0}}, {0.2f, {120, 10, 0}}, {0.4f, {230, 80, 0}}, {0.5f, {255, 230, 120}},
          {0.6f, {230, 80, 0}}, {0.8f, {120, 10, 0}}, {1.0f, {0, 0, 0}}}},
        {"Океан",
         {{0.0f, {0, 10, 40}}, {0.3f, {0, 80, 160}}, {0.5f, {120, 220, 240}},
          {0.7f, {0, 80, 160}}, {1.0f, {0, 10, 40}}}},
        {"Радуга",
         {{0.0f, {255, 0, 0}}, {1.0f / 6, {255, 255, 0}}, {2.0f / 6, {0, 255, 0}},
          {3.0f / 6, {0, 255, 255}}, {4.0f / 6, {0, 0, 255}}, {5.0f / 6, {255, 0, 255}},
          {1.0f, {255, 0, 0}}}},
        {"Рельеф",  // для Diamond-Square: вода, пляж, трава, скалы, снег
         {{0.0f, {30, 70, 140}}, {0.28f, {40, 110, 170}}, {0.32f, {210, 195, 140}},
          {0.40f, {60, 130, 60}}, {0.65f, {110, 95, 70}}, {0.85f, {150, 140, 130}},
          {1.0f, {250, 250, 250}}}}};
    return p;
}

std::uint8_t lerp8(std::uint8_t a, std::uint8_t b, float u) {
    return static_cast<std::uint8_t>(std::lround(a + (b - a) * u));
}
}  // namespace

ColorPalette::ColorPalette() { loadPreset(0); }

const char* ColorPalette::presetName(int id) {
    if (id < 0 || id >= kPresetCount) return "";
    return presets()[static_cast<std::size_t>(id)].name;
}

void ColorPalette::loadPreset(int id) {
    id = std::clamp(id, 0, kPresetCount - 1);
    stops_ = presets()[static_cast<std::size_t>(id)].stops;
    presetId_ = id;
}

void ColorPalette::clearStops() { stops_.clear(); }

void ColorPalette::addStop(float pos, Rgb color) {
    pos = std::clamp(pos, 0.0f, 1.0f);
    auto it = std::upper_bound(stops_.begin(), stops_.end(), pos,
                               [](float v, const ColorStop& s) { return v < s.pos; });
    stops_.insert(it, ColorStop{pos, color});
}

Rgb ColorPalette::colorAt(float t) const {
    if (stops_.empty()) return Rgb{};
    if (!(t > 0.0f)) t = 0.0f;  // отсекает и NaN
    if (t > 1.0f) t = 1.0f;
    if (mode_ == InterpMode::Logarithmic) t = std::log1p(kLogBeta * t) / std::log1p(kLogBeta);

    if (t <= stops_.front().pos) return stops_.front().color;
    if (t >= stops_.back().pos) return stops_.back().color;
    for (std::size_t i = 0; i + 1 < stops_.size(); ++i) {
        const ColorStop& a = stops_[i];
        const ColorStop& b = stops_[i + 1];
        if (t <= b.pos) {
            const float span = b.pos - a.pos;
            const float u = span > 1e-9f ? (t - a.pos) / span : 0.0f;
            return Rgb{lerp8(a.color.r, b.color.r, u), lerp8(a.color.g, b.color.g, u),
                       lerp8(a.color.b, b.color.b, u)};
        }
    }
    return stops_.back().color;
}

}  // namespace fractal

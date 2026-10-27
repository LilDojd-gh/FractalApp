#include "SettingsStorage.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <type_traits>

namespace fractal {

namespace {

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const auto a = s.find_first_not_of(ws);
    if (a == std::string::npos) return {};
    return s.substr(a, s.find_last_not_of(ws) - a + 1);
}

// Разбор чисел не зависит от текущей локали (в wxWidgets локаль может быть изменена).
template <class T>
bool parse(const std::string& s, T& out) {
    std::istringstream is(trim(s));
    is.imbue(std::locale::classic());
    T v{};
    is >> v;
    if (is.fail()) return false;
    out = v;
    return true;
}

template <class T>
std::string str(T v) {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os.precision(10);
    os << v;
    return os.str();
}

}  // namespace

bool SettingsStorage::save(const FractalParams& p, const std::string& path) {
    std::ofstream f(std::filesystem::u8path(path));
    if (!f) return false;
    f << "# FractalSurf settings\n"
      << "type=" << static_cast<int>(p.type) << "\n"
      << "iterations=" << p.maxIterations << "\n"
      << "julia_re=" << str(p.juliaRe) << "\n"
      << "julia_im=" << str(p.juliaIm) << "\n"
      << "roughness=" << str(p.roughness) << "\n"
      << "seed=" << p.seed << "\n"
      << "grid_level=" << p.gridLevel << "\n"
      << "palette=" << p.presetId << "\n"
      << "interp=" << static_cast<int>(p.interp) << "\n"
      << "inside_r=" << static_cast<int>(p.insideColor.r) << "\n"
      << "inside_g=" << static_cast<int>(p.insideColor.g) << "\n"
      << "inside_b=" << static_cast<int>(p.insideColor.b) << "\n"
      << "cycles=" << str(p.paletteCycles) << "\n"
      << "mode3d=" << (p.mode3D ? 1 : 0) << "\n"
      << "wireframe=" << (p.wireframe ? 1 : 0) << "\n"
      << "height_scale=" << str(p.heightScale) << "\n";
    return static_cast<bool>(f);
}

FractalParams SettingsStorage::load(const std::string& path, bool* ok) {
    FractalParams p;
    std::ifstream f(std::filesystem::u8path(path));
    if (ok) *ok = static_cast<bool>(f);
    if (!f) return p;

    std::map<std::string, std::string> kv;
    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        kv[trim(line.substr(0, eq))] = line.substr(eq + 1);
    }

    auto get = [&](const char* key, auto& dst) {
        auto it = kv.find(key);
        if (it == kv.end()) return;
        std::decay_t<decltype(dst)> v{};
        if (parse(it->second, v)) dst = v;
    };

    int t = static_cast<int>(p.type), interp = static_cast<int>(p.interp);
    int r = p.insideColor.r, g = p.insideColor.g, b = p.insideColor.b;
    int m3 = 0, wf = 0;
    get("type", t);            get("iterations", p.maxIterations);
    get("julia_re", p.juliaRe); get("julia_im", p.juliaIm);
    get("roughness", p.roughness); get("seed", p.seed);
    get("grid_level", p.gridLevel); get("palette", p.presetId);
    get("interp", interp);     get("cycles", p.paletteCycles);
    get("inside_r", r); get("inside_g", g); get("inside_b", b);
    get("mode3d", m3); get("wireframe", wf); get("height_scale", p.heightScale);

    p.type = (t >= 0 && t <= 2) ? static_cast<FractalType>(t) : FractalType::Mandelbrot;
    p.interp = (interp == 1) ? InterpMode::Logarithmic : InterpMode::Linear;
    auto byte = [](int v) { return static_cast<std::uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v)); };
    p.insideColor = Rgb{byte(r), byte(g), byte(b)};
    p.mode3D = m3 != 0;
    p.wireframe = wf != 0;
    p.clamp();
    return p;
}

std::vector<NamedPalette> SettingsStorage::loadPalettes(const std::string& path) {
    std::vector<NamedPalette> result;
    std::ifstream f(std::filesystem::u8path(path));
    if (!f) return result;

    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, '|')) parts.push_back(trim(part));
        if (parts.size() < 3 || parts[0].empty()) continue;  // нужно имя и минимум два узла

        NamedPalette np;
        np.name = parts[0];
        np.palette.clearStops();
        for (std::size_t i = 1; i < parts.size(); ++i) {
            std::istringstream is(parts[i]);
            is.imbue(std::locale::classic());
            float pos;
            int r, g, b;
            if (!(is >> pos >> r >> g >> b)) continue;
            auto byte = [](int v) { return static_cast<std::uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v)); };
            np.palette.addStop(pos, Rgb{byte(r), byte(g), byte(b)});
        }
        if (np.palette.stops().size() >= 2) result.push_back(std::move(np));
    }
    return result;
}

}  // namespace fractal

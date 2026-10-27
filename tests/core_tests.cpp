// Модульные тесты вычислительного ядра (без внешних фреймворков).
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Camera3D.h"
#include "ColorPalette.h"
#include "EngineFactory.h"
#include "FractalController.h"
#include "SettingsStorage.h"
#include "SurfaceMesh.h"

using namespace fractal;

static int g_failed = 0, g_checks = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(cond)) {                                                         \
            ++g_failed;                                                        \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                      \
    } while (0)

static void run(const char* name, const std::function<void()>& fn) {
    const int before = g_failed;
    fn();
    std::printf("[%s] %s\n", g_failed == before ? " OK " : "FAIL", name);
}

// ---------------------------------------------------------------------------
static void testHeightMap() {
    HeightMap m(4, 3);
    CHECK(m.width() == 4 && m.height() == 3);
    m.set(1, 1, 5.0f);
    m.set(2, 2, -3.0f);
    m.normalize();
    CHECK(m.at(1, 1) == 1.0f);
    CHECK(m.at(2, 2) == 0.0f);
    CHECK(m.at(0, 0) > 0.0f && m.at(0, 0) < 1.0f);

    HeightMap flat(5, 5);
    flat.normalize();
    CHECK(flat.at(2, 2) == 0.0f);  // постоянная карта -> нули, без деления на 0

    // Прореживание: 513 -> не более 257 по стороне.
    HeightMap big(513, 513);
    big.set(2, 4, 7.0f);
    const HeightMap d = big.downsampled(257);
    CHECK(d.width() == 257 && d.height() == 257);
    CHECK(d.at(1, 2) == 7.0f);
    CHECK(big.downsampled(1000).width() == 513);  // уже достаточно мала
}

static void testNormals() {
    // Плоская карта: нормаль строго вверх.
    HeightMap flat(8, 8);
    const glm::vec3 n0 = flat.normalAt(3, 3, 0.1f, 1.0f);
    CHECK(std::abs(n0.y - 1.0f) < 1e-6f && std::abs(n0.x) < 1e-6f);

    // Наклонная плоскость H = 0.1*x: dH/dx = 0.1/cell. Нормаль наклонена против роста высоты.
    HeightMap ramp(8, 8);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) ramp.set(x, y, 0.1f * x);
    for (int x : {0, 3, 7}) {  // в том числе на границе (односторонние разности)
        const glm::vec3 n = ramp.normalAt(x, 4, 1.0f, 1.0f);
        CHECK(std::abs(glm::length(n) - 1.0f) < 1e-5f);
        CHECK(n.x < 0.0f && std::abs(n.z) < 1e-6f);
        CHECK(std::abs(n.x / n.y + 0.1f) < 1e-5f);  // tg угла = -dH/dx
    }
}

static void testPalette() {
    ColorPalette p;
    p.clearStops();
    p.addStop(1.0f, Rgb{255, 255, 255});
    p.addStop(0.0f, Rgb{0, 0, 0});  // добавление не по порядку
    CHECK(p.stops().front().pos == 0.0f && p.stops().back().pos == 1.0f);
    CHECK(p.colorAt(0.0f).r == 0 && p.colorAt(1.0f).r == 255);
    CHECK(std::abs(p.colorAt(0.5f).g - 128) <= 1);
    CHECK(p.colorAt(-4.0f).r == 0 && p.colorAt(9.0f).r == 255);          // обрезка
    CHECK(p.colorAt(std::nanf("")).r == 0);                                  // NaN не ломает
    const int linear = p.colorAt(0.1f).r;
    p.setInterpolation(InterpMode::Logarithmic);
    CHECK(p.colorAt(0.1f).r > linear);                                       // малые t растянуты
    CHECK(p.colorAt(0.0f).r == 0 && p.colorAt(1.0f).r == 255);
    for (int id = 0; id < kPresetCount; ++id) {
        ColorPalette q;
        q.loadPreset(id);
        CHECK(q.stops().size() >= 2);
        CHECK(std::string(ColorPalette::presetName(id)).size() > 0);
    }
    ColorPalette q;
    q.loadPreset(99);  // некорректный номер приводится к допустимому
    CHECK(q.presetId() == kPresetCount - 1);
}

static void testEscapeTime() {
    // Известные точки множества Мандельброта.
    CHECK(EscapeTimeEngine::iterate({0, 0}, {0, 0}, 1000) < 0);        // c = 0 — внутри
    CHECK(EscapeTimeEngine::iterate({0, 0}, {-2, 0}, 1000) < 0);       // c = -2 — на границе, орбита ограничена
    CHECK(EscapeTimeEngine::iterate({0, 0}, {-1, 0}, 1000) < 0);       // период 2
    CHECK(EscapeTimeEngine::iterate({0, 0}, {1, 0}, 1000) > 0);        // быстро убегает
    CHECK(EscapeTimeEngine::iterate({0, 0}, {2, 2}, 1000) >= 0);       // убегает сразу

    // Гладкость: близкие точки дают близкие nu (нет ступенек целочисленного счётчика).
    double maxJump = 0;
    for (int i = 0; i < 200; ++i) {
        const double x = 0.30 + i * 1e-4;
        const double a = EscapeTimeEngine::iterate({0, 0}, {x, 0}, 5000);
        const double b = EscapeTimeEngine::iterate({0, 0}, {x + 1e-4, 0}, 5000);
        if (a > 0 && b > 0) maxJump = std::max(maxJump, std::abs(a - b) / std::max(a, b));
    }
    CHECK(maxJump < 0.2);

    // Быстрая проверка кардиоиды/луковицы не противоречит прямому счёту.
    MandelbrotEngine eng;
    eng.setMaxIterations(2000);
    int wrong = 0, quick = 0;
    for (int j = 0; j < 120; ++j)
        for (int i = 0; i < 160; ++i) {
            const Complex p(-2.2 + 3.2 * i / 160.0, -1.2 + 2.4 * j / 120.0);
            const double x = p.real(), y = p.imag(), xq = x - 0.25, q = xq * xq + y * y;
            const bool fast = q * (q + xq) < 0.25 * y * y || (x + 1) * (x + 1) + y * y < 0.0625;
            if (fast) {
                ++quick;
                if (EscapeTimeEngine::iterate({0, 0}, p, 2000) >= 0) ++wrong;
            }
        }
    CHECK(quick > 500 && wrong == 0);
}

static void testMandelbrotMap() {
    MandelbrotEngine eng;
    eng.setMaxIterations(300);
    HeightMap m(160, 120);
    eng.compute(m);
    // Центр кадра (-0.5, 0) лежит внутри главной кардиоиды; угол (-2.25, 1.3) — снаружи.
    CHECK(m.isInside(80, 60));
    CHECK(!m.isInside(0, 0));
    int inside = 0;
    for (int y = 0; y < 120; ++y)
        for (int x = 0; x < 160; ++x) {
            CHECK(m.at(x, y) >= 0.0f && m.at(x, y) <= 1.0f);
            inside += m.isInside(x, y);
        }
    CHECK(inside > 300 && inside < 160 * 120 / 2);
    // Симметрия множества относительно вещественной оси (центр на оси, чётное число строк).
    int asym = 0;
    for (int y = 0; y < 60; ++y)
        for (int x = 0; x < 160; ++x)
            if (m.isInside(x, y) != m.isInside(x, 119 - y)) ++asym;
    CHECK(asym < 20);
}

static void testJulia() {
    JuliaEngine eng;
    eng.setMaxIterations(200);
    eng.setConstant({-0.8, 0.156});
    HeightMap m(120, 120);
    eng.compute(m);
    // Множество Жюлиа центрально-симметрично: J(z) = J(-z).
    int diff = 0;
    for (int y = 0; y < 120; ++y)
        for (int x = 0; x < 120; ++x)
            if (m.isInside(x, y) != m.isInside(119 - x, 119 - y)) ++diff;
    CHECK(diff < 0.01 * 120 * 120);
    // Другая константа даёт другую картинку.
    JuliaEngine eng2;
    eng2.setMaxIterations(200);
    eng2.setConstant({0.285, 0.01});
    HeightMap m2(120, 120);
    eng2.compute(m2);
    int changed = 0;
    for (int y = 0; y < 120; ++y)
        for (int x = 0; x < 120; ++x) changed += std::abs(m.at(x, y) - m2.at(x, y)) > 1e-4f;
    CHECK(changed > 1000);
}

static void testNavigation() {
    MandelbrotEngine eng;
    HeightMap m(200, 100);
    eng.compute(m);  // размер известен только вызывающему
    const int W = 200, H = 100;

    // Масштабирование относительно курсора: точка под курсором не смещается.
    const int px = 37, py = 71;
    const Complex before = eng.pixelToComplex(px, py, W, H);
    eng.zoomAt(px, py, 8.0, W, H);
    const Complex after = eng.pixelToComplex(px, py, W, H);
    CHECK(std::abs(before - after) < 1e-12);
    CHECK(std::abs(eng.zoomFactor() - 8.0) < 1e-9);

    // Панорамирование: точка, бывшая под (100, 50), оказывается под (110, 45) после сдвига мыши.
    const Complex p0 = eng.pixelToComplex(100, 50, W, H);
    eng.pan(10, -5, W);
    CHECK(std::abs(eng.pixelToComplex(110, 45, W, H) - p0) < 1e-12);

    // Пределы масштаба и сброс.
    eng.zoomAt(100, 50, 1e30, W, H);
    CHECK(eng.zoomFactor() <= 1.0000001e13);
    eng.zoomAt(100, 50, 1e-30, W, H);
    CHECK(eng.zoomFactor() >= 0.2499);
    eng.resetView();
    CHECK(std::abs(eng.zoomFactor() - 1.0) < 1e-12);
    CHECK(std::abs(eng.center() - Complex(-0.5, 0.0)) < 1e-12);
    eng.setZoomFactor(1e6);
    CHECK(std::abs(eng.zoomFactor() - 1e6) < 1e-3);
}

static double meanRoughness(const HeightMap& m) {
    double s = 0;
    long n = 0;
    for (int y = 0; y < m.height(); ++y)
        for (int x = 0; x + 1 < m.width(); ++x) {
            s += std::abs(m.at(x, y) - m.at(x + 1, y));
            ++n;
        }
    return s / static_cast<double>(n);
}

static void testDiamondSquare() {
    DiamondSquareEngine a;
    a.setGridLevel(7);
    a.setSeed(123);
    a.setRoughness(0.5f);
    HeightMap m1, m2, m3;
    a.compute(m1);
    CHECK(m1.width() == 129 && m1.height() == 129);
    float lo = 1e9f, hi = -1e9f;
    for (int y = 0; y < 129; ++y)
        for (int x = 0; x < 129; ++x) {
            lo = std::min(lo, m1.at(x, y));
            hi = std::max(hi, m1.at(x, y));
            CHECK(!m1.isInside(x, y));
        }
    CHECK(std::abs(lo) < 1e-6f && std::abs(hi - 1.0f) < 1e-6f);

    a.compute(m2);  // тот же seed -> тот же результат
    bool same = true;
    for (int y = 0; y < 129 && same; ++y)
        for (int x = 0; x < 129; ++x)
            if (m1.at(x, y) != m2.at(x, y)) { same = false; break; }
    CHECK(same);

    a.setSeed(124);
    a.compute(m3);
    CHECK(m3.at(64, 64) != m1.at(64, 64) || m3.at(10, 20) != m1.at(10, 20));

    // Больший параметр Херста — более гладкая поверхность (среднее по нескольким seed).
    double rough = 0, smooth = 0;
    for (std::uint32_t s = 1; s <= 5; ++s) {
        DiamondSquareEngine e;
        e.setGridLevel(7);
        e.setSeed(s);
        HeightMap r, g;
        e.setRoughness(0.15f);
        e.compute(r);
        e.setRoughness(0.95f);
        e.compute(g);
        rough += meanRoughness(r);
        smooth += meanRoughness(g);
    }
    CHECK(smooth < rough);

    // Размеры карты: 2^n + 1.
    for (int n = 6; n <= 8; ++n) {
        DiamondSquareEngine e;
        e.setGridLevel(n);
        HeightMap h;
        e.compute(h);
        CHECK(h.width() == (1 << n) + 1 && h.height() == h.width());
    }
}

static void testMesh() {
    // Наклонная плоскость 33x33: проверяем размеры, нормали вверх, обход против часовой стрелки.
    HeightMap m(33, 33);
    for (int y = 0; y < 33; ++y)
        for (int x = 0; x < 33; ++x) m.set(x, y, x / 32.0f);
    SurfaceMesh mesh;
    mesh.build(m, 0.5f, [](float h, bool) { return Rgb{static_cast<std::uint8_t>(h * 255), 0, 0}; });
    CHECK(mesh.columns() == 33 && mesh.rows() == 33);
    CHECK(mesh.vertices().size() == 33u * 33u);
    CHECK(mesh.indices().size() == 6u * 32u * 32u);
    for (auto idx : mesh.indices()) CHECK(idx < mesh.vertices().size());

    int facingUp = 0;
    const auto& v = mesh.vertices();
    const auto& ix = mesh.indices();
    for (std::size_t t = 0; t < ix.size(); t += 3) {
        const glm::vec3 e1 = v[ix[t + 1]].position - v[ix[t]].position;
        const glm::vec3 e2 = v[ix[t + 2]].position - v[ix[t]].position;
        if (glm::cross(e1, e2).y > 0) ++facingUp;
    }
    CHECK(facingUp == static_cast<int>(ix.size() / 3));
    CHECK(std::abs(v.front().position.x + 1.0f) < 1e-5f && std::abs(v.back().position.x - 1.0f) < 1e-5f);
    CHECK(v.front().position.y == 0.0f && std::abs(v[32].position.y - 0.5f) < 1e-5f);
    CHECK(v[5 * 33 + 10].normal.x < 0.0f && v[5 * 33 + 10].normal.y > 0.0f);

    // Карта с множеством: точки «внутри» лежат на нулевом уровне и красятся цветом множества.
    HeightMap s(20, 20);
    for (int y = 0; y < 20; ++y)
        for (int x = 0; x < 20; ++x) {
            const bool in = (x - 10) * (x - 10) + (y - 10) * (y - 10) < 16;
            s.setInside(x, y, in);
            s.set(x, y, in ? 0.0f : 0.2f + 0.8f * x / 19.0f);
        }
    SurfaceMesh m2;
    m2.build(s, 1.0f, [](float, bool in) { return in ? Rgb{9, 9, 9} : Rgb{200, 0, 0}; });
    CHECK(m2.vertices()[10 * 20 + 10].position.y == 0.0f);
    CHECK(m2.vertices()[10 * 20 + 10].color.x < 0.05f);
    CHECK(m2.vertices()[0].position.y == 0.0f);                         // минимум вне множества
    CHECK(std::abs(m2.vertices()[19].position.y - 1.0f) < 1e-5f);       // максимум
    // Вырожденные входы не падают.
    SurfaceMesh m3;
    m3.build(HeightMap(1, 1), 1.0f, [](float, bool) { return Rgb{}; });
    CHECK(m3.empty());
}

static void testCamera() {
    Camera3D c;
    CHECK(std::abs(glm::length(c.position() - c.target()) - c.distance()) < 1e-4f);
    c.rotate(0, 500);
    CHECK(c.pitch() <= 89.0f);
    c.rotate(0, -999);
    CHECK(c.pitch() >= 2.0f);
    c.rotate(400, 0);
    CHECK(c.yaw() >= -180.0f && c.yaw() <= 180.0f);
    const float d0 = c.distance();
    c.zoom(1);
    CHECK(c.distance() < d0);
    c.zoom(1000);
    CHECK(c.distance() >= 0.6f);
    c.zoom(-1000);
    CHECK(c.distance() <= 15.0f);
    // Цель находится по центру экрана.
    const glm::vec4 clip = c.projectionMatrix(1.5f) * c.viewMatrix() * glm::vec4(c.target(), 1.0f);
    CHECK(std::abs(clip.x / clip.w) < 1e-5f && std::abs(clip.y / clip.w) < 1e-5f);
}

static void testSettings() {
    const std::string path = "fractalsurf_test_settings.ini";  // временный файл в рабочей папке
    FractalParams p;
    p.type = FractalType::Julia;
    p.maxIterations = 1234;
    p.juliaRe = -0.7269;
    p.juliaIm = 0.1889;
    p.roughness = 0.35f;
    p.seed = 4000000000u;
    p.gridLevel = 9;
    p.presetId = 3;
    p.interp = InterpMode::Logarithmic;
    p.insideColor = Rgb{10, 20, 30};
    p.paletteCycles = 12.5f;
    p.mode3D = true;
    p.wireframe = true;
    p.heightScale = 0.9f;
    CHECK(SettingsStorage::save(p, path));
    bool ok = false;
    const FractalParams q = SettingsStorage::load(path, &ok);
    CHECK(ok);
    CHECK(q.type == p.type && q.maxIterations == 1234 && q.gridLevel == 9 && q.presetId == 3);
    CHECK(std::abs(q.juliaRe - p.juliaRe) < 1e-9 && std::abs(q.juliaIm - p.juliaIm) < 1e-9);
    CHECK(std::abs(q.roughness - 0.35f) < 1e-6f && q.seed == 4000000000u);
    CHECK(q.interp == InterpMode::Logarithmic && q.insideColor.g == 20);
    CHECK(q.mode3D && q.wireframe && std::abs(q.heightScale - 0.9f) < 1e-6f);

    // Повреждённый файл: мусор, выход за диапазон, неизвестные ключи — без падения, значения по умолчанию.
    {
        std::ofstream f(path);
        f << "garbage line\niterations=999999\ntype=77\nroughness=abc\nroughness=\n=5\n"
             "julia_re=NaN\ngrid_level=-3\nunknown=1\ninside_r=300\n";
    }
    const FractalParams g = SettingsStorage::load(path, &ok);
    CHECK(ok);
    CHECK(g.maxIterations == 5000 && g.type == FractalType::Mandelbrot);
    CHECK(g.gridLevel == 6 && std::abs(g.roughness - 0.8f) < 1e-6f);
    CHECK(std::isfinite(g.juliaRe) && g.insideColor.r == 255);

    bool ok2 = true;
    const FractalParams h = SettingsStorage::load("/nonexistent/dir/none.ini", &ok2);
    CHECK(!ok2 && h.maxIterations == 500);
    std::remove(path.c_str());

    // Палитры пользователя.
    const std::string pp = "fractalsurf_test_pal.txt";
    {
        std::ofstream f(pp);
        f << "# comment\nЗакат | 0.0 20 0 60 | 1.0 255 240 190\nПлохая | 0.5 1 2 3\n\nОдна строка без узлов\n"
             "Градиент | 1.0 255 255 255 | 0.0 0 0 0 | x y z\n";
    }
    const auto pals = SettingsStorage::loadPalettes(pp);
    CHECK(pals.size() == 2);
    if (pals.size() == 2) {
        CHECK(pals[0].name == "Закат" && pals[0].palette.colorAt(0.0f).b == 60);
        CHECK(pals[1].palette.stops().front().pos == 0.0f);  // узлы упорядочены
    }
    std::remove(pp.c_str());
    CHECK(SettingsStorage::loadPalettes("/nonexistent.txt").empty());
}

static void testController() {
    FractalController c;
    CHECK(c.params().type == FractalType::Mandelbrot);
    c.setResolution(120, 80);
    c.recompute();
    CHECK(c.hasData() && c.heightMap().width() == 120 && c.heightMap().height() == 80);

    // Смена типа заменяет генератор.
    FractalParams p = c.params();
    p.type = FractalType::DiamondSquare;
    p.gridLevel = 6;
    c.setParams(p);
    CHECK(!c.supportsNavigation());
    c.recompute();
    CHECK(c.heightMap().width() == 65);
    p.type = FractalType::Mandelbrot;
    c.setParams(p);
    CHECK(c.supportsNavigation());
    c.recompute();
    CHECK(c.heightMap().width() == 120);  // карта вернулась к размеру растра

    // Навигация: масштаб и сдвиг меняют описание вида; сброс возвращает исходное.
    const std::string v0 = c.describeView();
    c.zoomAt(60, 40, 4.0);
    CHECK(c.describeView() != v0 && std::abs(c.zoomFactor() - 4.0) < 1e-9);
    c.pan(15, 15);
    c.resetView();
    CHECK(c.describeView() == v0);

    // 2D-изображение: размер и согласованность цвета с палитрой.
    c.recompute();
    const Image2D img = c.render2D();
    CHECK(img.width() == 120 && img.height() == 80);
    CHECK(img.byteSize() == 120u * 80u * 3u);
    const Rgb inside = img.pixel(60, 40);  // центр кадра внутри множества
    CHECK(inside.r == 0 && inside.g == 0 && inside.b == 0);

    // Смена палитры/цвета внутри перекрашивает без пересчёта.
    FractalParams a = c.params();
    a.insideColor = Rgb{1, 2, 3};
    a.presetId = 2;
    c.updateAppearance(a);
    const Image2D img2 = c.render2D();
    CHECK(img2.pixel(60, 40).b == 3);
    CHECK(c.params().maxIterations == 500);

    // Сетка для 3D и вырожденное разрешение.
    c.buildMesh();
    CHECK(!c.mesh().empty());
    c.setResolution(0, -5);
    c.recompute();
    CHECK(c.heightMap().width() >= 16);

    // Некорректные параметры приводятся к допустимым.
    FractalParams bad;
    bad.maxIterations = -50;
    bad.presetId = 77;
    bad.gridLevel = 99;
    bad.heightScale = 1e9f;
    c.setParams(bad);
    CHECK(c.params().maxIterations == 10 && c.params().presetId == kPresetCount - 1);
    CHECK(c.params().gridLevel == 10 && c.params().heightScale <= 1.5f);
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // русский текст в консоли Windows
#endif
    run("HeightMap: размер, нормировка, прореживание", testHeightMap);
    run("HeightMap: нормали (плоскость, наклон, граница)", testNormals);
    run("ColorPalette: узлы, интерполяция, пресеты", testPalette);
    run("Escape-Time: известные точки, гладкость, кардиоида", testEscapeTime);
    run("Мандельброт: карта высот и симметрия", testMandelbrotMap);
    run("Жюлиа: центральная симметрия, зависимость от c", testJulia);
    run("Навигация: zoom вокруг курсора, pan, пределы", testNavigation);
    run("Diamond-Square: размер, seed, шероховатость", testDiamondSquare);
    run("SurfaceMesh: индексы, нормали, обход, множество", testMesh);
    run("Camera3D: углы, дистанция, проекция", testCamera);
    run("SettingsStorage: сохранение, повреждённые файлы, палитры", testSettings);
    run("FractalController: сценарий целиком", testController);
    std::printf("\nПроверок: %d, не пройдено: %d\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
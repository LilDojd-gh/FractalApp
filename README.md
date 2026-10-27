# FractalSurf — построение фрактальных поверхностей

Курсовая работа по компьютерной графике, вариант 13. C++17, wxWidgets 3.2, OpenGL 3.3, GLM, GLEW, CMake.

## Состав
- `core/` — вычислительное ядро (без wxWidgets и OpenGL): генераторы Мандельброта, Жюлиа, Diamond-Square, `HeightMap`, `ColorPalette`, `Camera3D`, `SurfaceMesh`, `FractalController`, `SettingsStorage`.
- `render/` — слой OpenGL (`SurfaceRenderer`, шейдеры внутри .cpp).
- `gui/` — интерфейс wxWidgets: `MainWindow`, `ControlPanel`, `Canvas2D`, `Canvas3D`.
- `tests/core_tests.cpp` — модульные тесты ядра.
- `data/palettes.txt` — формат пользовательских палитр (загрузка в интерфейс пока не подключена).

## Сборка
Ubuntu / Debian:
    sudo apt install build-essential cmake libwxgtk3.2-dev libglm-dev libglew-dev libgl-dev
    cmake -S . -B build && cmake --build build -j
    ./build/core_tests && ./build/fractalsurf

Windows (Visual Studio + vcpkg): `vcpkg install wxwidgets glm glew`, затем
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
    cmake --build build --config Release

Только ядро и тесты (без GUI): `cmake -S . -B build -DBUILD_GUI=OFF`.

Если на Linux под Wayland не создаётся контекст OpenGL: `GDK_BACKEND=x11 ./build/fractalsurf`.

## Управление
Колесо мыши — масштаб вокруг курсора, левая кнопка — сдвиг (2D) или вращение камеры (3D), Enter / Ctrl+R — рендеринг, Ctrl+S — сохранить, Ctrl+0 — сбросить вид, F1 — справка.

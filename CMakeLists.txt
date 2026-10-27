cmake_minimum_required(VERSION 3.16)
project(FractalSurf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)   # курсовая сдаётся со сборкой Release
endif()

if(MSVC)
  add_compile_options(/utf-8 /W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

option(BUILD_GUI   "Собирать графическое приложение (wxWidgets + OpenGL)" ON)
option(BUILD_TESTS "Собирать тесты ядра" ON)

find_package(Threads REQUIRED)
find_package(glm CONFIG REQUIRED)

# ---------- вычислительное ядро: не зависит от wxWidgets и OpenGL ----------
add_library(fractal_core STATIC
        core/HeightMap.cpp
        core/ColorPalette.cpp
        core/Camera3D.cpp
        core/SurfaceMesh.cpp
        core/EscapeTimeEngine.cpp
        core/DiamondSquareEngine.cpp
        core/FractalController.cpp
        core/SettingsStorage.cpp)
target_include_directories(fractal_core PUBLIC core)
target_link_libraries(fractal_core PUBLIC glm::glm Threads::Threads)

# ---------- тесты ядра ----------
if(BUILD_TESTS)
  enable_testing()
  add_executable(core_tests tests/core_tests.cpp)
  target_link_libraries(core_tests PRIVATE fractal_core)
  add_test(NAME core_tests COMMAND core_tests)
endif()

# ---------- графическое приложение ----------
if(BUILD_GUI)
  # 1) конфиг-файл wxWidgets (vcpkg, MSYS2): цели wx::core, wx::base, wx::gl
  find_package(wxWidgets CONFIG QUIET)
  if(wxWidgets_FOUND AND TARGET wx::core)
    set(WX_LIBS wx::core wx::base wx::gl)
  else()
    # 2) классический поиск через wx-config (Linux, MSYS2)
    find_package(wxWidgets REQUIRED COMPONENTS core base gl)
    include(${wxWidgets_USE_FILE})
    set(WX_LIBS ${wxWidgets_LIBRARIES})
  endif()

  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)

  add_executable(fractalsurf WIN32
          gui/App.cpp
          gui/MainWindow.cpp
          gui/ControlPanel.cpp
          gui/Canvas2D.cpp
          gui/Canvas3D.cpp
          render/SurfaceRenderer.cpp)
  target_include_directories(fractalsurf PRIVATE gui render)
  target_link_libraries(fractalsurf PRIVATE fractal_core ${WX_LIBS} OpenGL::GL GLEW::GLEW)

  # MinGW: runtime-библиотеки GCC линкуем статически, чтобы .exe меньше зависел от DLL
  if(MINGW)
    target_link_options(fractalsurf PRIVATE -static-libgcc -static-libstdc++)
  endif()

  # Файлы данных (палитры) копируются рядом с исполняемым файлом
  add_custom_command(TARGET fractalsurf POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:fractalsurf>/data)
endif()
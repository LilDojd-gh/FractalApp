cmake_minimum_required(VERSION 3.16)
project(FractalSurf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)   # курсовая сдаётся со сборкой RELEASE
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
  core/HeightMap.cpp core/ColorPalette.cpp core/Camera3D.cpp core/SurfaceMesh.cpp
  core/EscapeTimeEngine.cpp core/DiamondSquareEngine.cpp
  core/FractalController.cpp core/SettingsStorage.cpp)
target_include_directories(fractal_core PUBLIC core)
target_link_libraries(fractal_core PUBLIC glm::glm Threads::Threads)

if(BUILD_TESTS)
  enable_testing()
  add_executable(core_tests tests/core_tests.cpp)
  target_link_libraries(core_tests PRIVATE fractal_core)
  add_test(NAME core_tests COMMAND core_tests)
endif()

# ---------- графическое приложение ----------
if(BUILD_GUI)
  find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base gl)
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  include(${wxWidgets_USE_FILE})

  add_executable(fractalsurf WIN32
    gui/App.cpp gui/MainWindow.cpp gui/ControlPanel.cpp gui/Canvas2D.cpp gui/Canvas3D.cpp
    render/SurfaceRenderer.cpp)
  target_include_directories(fractalsurf PRIVATE gui render)
  target_link_libraries(fractalsurf PRIVATE fractal_core ${wxWidgets_LIBRARIES} OpenGL::GL GLEW::GLEW)

  # Файлы данных (пресеты палитр) копируются рядом с исполняемым файлом.
  add_custom_command(TARGET fractalsurf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:fractalsurf>/data)
endif()

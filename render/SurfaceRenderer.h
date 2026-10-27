// SurfaceRenderer.h — слой OpenGL: шейдеры, буферы и отрисовка сетки поверхности.
// Заголовок намеренно не включает GL-заголовки (их подключает только .cpp),
// чтобы не конфликтовать с GLEW и wxGLCanvas.
#pragma once
#include <string>
#include <vector>

#include "Camera3D.h"
#include "SurfaceMesh.h"

namespace fractal {

/**
 * Все методы, кроме конструктора, вызываются при активном контексте OpenGL
 * (wxGLCanvas::SetCurrent). Освещение по Ламберту считается в вершинном шейдере:
 * I = I_a*k_a + I_d*k_d*max(0, N·L), цвет вершины умножается на I.
 */
class SurfaceRenderer {
public:
    SurfaceRenderer() = default;
    ~SurfaceRenderer() = default;  // ресурсы GPU освобождает release() при активном контексте
    SurfaceRenderer(const SurfaceRenderer&) = delete;
    SurfaceRenderer& operator=(const SurfaceRenderer&) = delete;

    /// Инициализирует GLEW, компилирует шейдеры, создаёт буферы. При ошибке возвращает false и текст в err.
    bool init(std::string& err);
    bool ready() const { return ready_; }
    void release();

    void upload(const SurfaceMesh& mesh);
    void beginFrame(int width, int height);  ///< viewport + очистка буферов
    void draw(const Camera3D& cam, float aspect, bool wireframe);
    /// Читает кадр из заднего буфера: RGB, строки снизу вверх.
    bool readPixels(int width, int height, std::vector<unsigned char>& rgb);

private:
    bool ready_ = false;
    unsigned program_ = 0, vao_ = 0, vbo_ = 0, ebo_ = 0;
    int mvpLocation_ = -1;
    int indexCount_ = 0;
};

}  // namespace fractal

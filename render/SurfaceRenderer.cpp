#include "SurfaceRenderer.h"

#include <GL/glew.h>

#include <glm/gtc/type_ptr.hpp>

namespace fractal {

namespace {

const char* kVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    // Модель — единичная матрица, поэтому нормаль уже в мировых координатах.
    const vec3  L  = normalize(vec3(0.45, 0.80, 0.35));  // направление на источник света
    const float Ia = 1.0, ka = 0.28;                      // фоновая составляющая
    const float Id = 1.0, kd = 0.85;                      // диффузная составляющая
    float I = Ia * ka + Id * kd * max(dot(normalize(aNormal), L), 0.0);
    vColor = clamp(aColor * I, 0.0, 1.0);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char* kFragmentShader = R"(#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, 1.0); }
)";

bool compile(GLuint shader, const char* src, std::string& err) {
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok) return true;
    char log[1024] = {};
    glGetShaderInfoLog(shader, sizeof(log) - 1, nullptr, log);
    err = std::string("Ошибка компиляции шейдера: ") + log;
    return false;
}

}  // namespace

bool SurfaceRenderer::init(std::string& err) {
    if (ready_) return true;
    glewExperimental = GL_TRUE;  // необходимо для core-профиля
    const GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        err = std::string("Не удалось инициализировать GLEW: ") +
              reinterpret_cast<const char*>(glewGetErrorString(glewStatus));
        return false;
    }
    glGetError();  // glewInit в core-профиле может оставить GL_INVALID_ENUM

    GLuint vs = glCreateShader(GL_VERTEX_SHADER), fs = glCreateShader(GL_FRAGMENT_SHADER);
    const bool compiled = compile(vs, kVertexShader, err) && compile(fs, kFragmentShader, err);
    if (!compiled) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024] = {};
        glGetProgramInfoLog(program_, sizeof(log) - 1, nullptr, log);
        err = std::string("Ошибка компоновки шейдеров: ") + log;
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }
    mvpLocation_ = glGetUniformLocation(program_, "uMVP");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    const GLsizei stride = sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, color)));
    glBindVertexArray(0);

    ready_ = true;
    return true;
}

void SurfaceRenderer::release() {
    if (!ready_) return;
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(program_);
    vbo_ = ebo_ = vao_ = program_ = 0;
    indexCount_ = 0;
    ready_ = false;
}

void SurfaceRenderer::upload(const SurfaceMesh& mesh) {
    if (!ready_) return;
    indexCount_ = static_cast<int>(mesh.indices().size());
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices().size() * sizeof(Vertex)),
                 mesh.vertices().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.indices().size() * sizeof(std::uint32_t)),
                 mesh.indices().data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void SurfaceRenderer::beginFrame(int width, int height) {
    glViewport(0, 0, width, height);
    glClearColor(0.125f, 0.14f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SurfaceRenderer::draw(const Camera3D& cam, float aspect, bool wireframe) {
    if (!ready_ || indexCount_ == 0) return;
    const glm::mat4 mvp = cam.projectionMatrix(aspect) * cam.viewMatrix();
    glEnable(GL_DEPTH_TEST);
    if (wireframe) {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glEnable(GL_CULL_FACE);  // грани ориентированы против часовой стрелки при взгляде сверху
        glCullFace(GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glUseProgram(program_);
    glUniformMatrix4fv(mvpLocation_, 1, GL_FALSE, glm::value_ptr(mvp));
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

bool SurfaceRenderer::readPixels(int width, int height, std::vector<unsigned char>& rgb) {
    if (!ready_ || width <= 0 || height <= 0) return false;
    rgb.assign(static_cast<std::size_t>(width) * height * 3, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
    return glGetError() == GL_NO_ERROR;
}

}  // namespace fractal

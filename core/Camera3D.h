// Camera3D.h — орбитальная камера: сферические координаты вокруг точки-цели.
#pragma once
#include <glm/glm.hpp>

namespace fractal {

class Camera3D {
public:
    Camera3D() { reset(); }

    void reset();
    /// Поворот на dYaw, dPitch градусов; угол места ограничен диапазоном [2°, 89°].
    void rotate(float dYaw, float dPitch);
    /// Приближение (delta > 0) или удаление (delta < 0); один шаг — изменение дистанции на 10 %.
    void zoom(float delta);

    glm::vec3 position() const;
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;

    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
    float distance() const { return distance_; }
    glm::vec3 target() const { return target_; }

private:
    float yaw_ = 0, pitch_ = 0, distance_ = 0;  // градусы, градусы, ед. сцены
    glm::vec3 target_{0.0f, 0.2f, 0.0f};
};

}  // namespace fractal

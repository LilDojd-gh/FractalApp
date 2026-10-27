#include "Camera3D.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace fractal {

void Camera3D::reset() {
    yaw_ = -40.0f;
    pitch_ = 35.0f;
    distance_ = 3.2f;
}

void Camera3D::rotate(float dYaw, float dPitch) {
    yaw_ += dYaw;
    if (yaw_ > 180.0f) yaw_ -= 360.0f;
    if (yaw_ < -180.0f) yaw_ += 360.0f;
    pitch_ = std::clamp(pitch_ + dPitch, 2.0f, 89.0f);
}

void Camera3D::zoom(float delta) {
    distance_ = std::clamp(distance_ * std::pow(0.9f, delta), 0.6f, 15.0f);
}

glm::vec3 Camera3D::position() const {
    const float y = glm::radians(yaw_), p = glm::radians(pitch_);
    return target_ + distance_ * glm::vec3(std::cos(p) * std::sin(y), std::sin(p), std::cos(p) * std::cos(y));
}

glm::mat4 Camera3D::viewMatrix() const { return glm::lookAt(position(), target_, glm::vec3(0, 1, 0)); }

glm::mat4 Camera3D::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(45.0f), aspect > 0.01f ? aspect : 1.0f, 0.05f, 100.0f);
}

}  // namespace fractal

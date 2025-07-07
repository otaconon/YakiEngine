#pragma once

#include <glm/glm.hpp>

#include "../Transform.h"

class Camera {
public:
    Camera();

    glm::mat4 GetPerspectiveProjection() const;
    glm::mat4 GetOrthographicProjection(float halfWidth, float halfHeight) const;
    glm::mat4 GetView(const glm::mat4& model) const;
    glm::ivec2 GetAspectRatio() const;
private:
    float m_near;
    float m_far;
    float m_fovx;
    glm::ivec2 m_aspectRatio;
};

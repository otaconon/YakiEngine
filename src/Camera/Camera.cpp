#include "Camera.h"

#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>

Camera::Camera()
    : m_near(0.1f),
    m_far(100.f),
    m_fovx(90.f),
    m_aspectRatio(1600, 900)
{
}

glm::mat4 Camera::GetPerspectiveProjection() const
{
    const float sx = 1.0f / std::tan(m_fovx * 0.5f);
    const float sy = sx * (static_cast<float>(m_aspectRatio.x) / static_cast<float>(m_aspectRatio.y));

    glm::mat4 P(0.0f);
    P[0][0] = sx;
    P[1][1] = -sy;
    P[2][2] = -m_far / (m_far - m_near);
    P[2][3] = -1.0f;
    P[3][2] = -(m_far * m_near) / (m_far - m_near);

    return P;
}

glm::mat4 Camera::GetOrthographicProjection(const float halfWidth, const float halfHeight) const
{
    const float left = -halfWidth, bottom = halfHeight;
    const float right = halfWidth, top = -halfHeight;

    glm::mat4 projection(0.f);
    projection[0][0] = 2.f/(right - left);
    projection[1][1] = 2.f/(top - bottom);
    projection[2][2] = -2.f/(m_far - m_near);
    projection[3] = {-(right+left)/(right-left), -(top+bottom)/(top-bottom), -(m_far + m_near)/(m_far-m_near), 1};

    return projection;
}

glm::mat4 Camera::GetView(const glm::mat4& model) const { return glm::inverse(model); }
glm::ivec2 Camera::GetAspectRatio() const { return m_aspectRatio; }

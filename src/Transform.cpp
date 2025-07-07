#include "Transform.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Vulkan/Buffer.h"
#include "Vulkan/Buffer.h"

Transform::Transform()
    : m_position{},
    m_rotation{},
    m_size{}
{
}

Transform::Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    : m_position(position),
    m_rotation(rotation),
    m_size(scale)
{
}

void Transform::Translate(const glm::vec3& vector)
{
    m_position += vector;
}

void Transform::Rotate(const glm::vec3& axis, float angle)
{
    const glm::vec3 normAxis = glm::normalize(axis);
    const glm::quat q = glm::angleAxis(angle, normAxis);
    m_rotation = glm::normalize(q * m_rotation);
}

void Transform::Scale(const glm::vec3& vector)
{
    m_size += vector;
}

glm::vec3 Transform::GetPosition() const { return m_position; }
glm::quat Transform::GetRotation() const { return m_rotation; }
glm::vec3 Transform::GetSize() const { return m_size; }
glm::mat4 Transform::GetModel() const { return translate(rotate(scale(glm::mat4(1.0f), m_size), m_rotation), m_position); }
glm::vec3 Transform::GetForward() const { return m_rotation * glm::vec3(0.f, 0.f, -1.f); }
glm::vec3 Transform::GetUp() const { return m_rotation * glm::vec3(0.f, -1.f, 0.f); }
glm::vec3 Transform::GetRight() const { return m_rotation * glm::vec3(1.f, 0.f, 0.f); }

void Transform::SetPosition(const glm::vec3& position) { m_position = position; }
void Transform::SetRotation(const glm::quat& q) { m_rotation = q; }
void Transform::SetSize(const glm::vec3& size) { m_size = size; }

void Transform::LookAt(const glm::vec3& point)
{
    glm::vec3 dir = point - m_position;
    float len = glm::length(dir);
    if (len < 1e-4f)
        return;

    dir /= len;
    glm::vec3 up(0.f, 1.f, 0.f);
    if (glm::abs(glm::dot(dir, up)) > 0.999f)
        up = glm::vec3(1.f, 0.f, 0.f);

    m_rotation = glm::quatLookAt(dir, up);
}

glm::mat4 Transform::translate(glm::mat4&& model, const glm::vec3& vector)
{
    model[3][0] += vector.x;
    model[3][1] += vector.y;
    model[3][2] += vector.z;
    return model;
}

glm::mat4 Transform::rotate(glm::mat4&& model, const glm::quat q)
{
    glm::mat4 rot = glm::toMat4(q);
    return rot * model;
}

glm::mat4 Transform::scale(glm::mat4&& model, const glm::vec3& vector)
{
    model[0][0] += vector.x;
    model[1][1] += vector.y;
    model[2][2] += vector.z;
    return model;
}

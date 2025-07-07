#pragma once
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

class Transform 
{
public:
    Transform();
    Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);

    void Translate(const glm::vec3& vector);
    void Rotate(const glm::vec3& axis, float angle);
    void Scale(const glm::vec3& vector);

    glm::vec3 GetPosition() const;
    glm::quat GetRotation() const;
    glm::vec3 GetSize() const;
    glm::mat4 GetModel() const;

    glm::vec3 GetForward() const;
    glm::vec3 GetUp() const;
    glm::vec3 GetRight() const;

    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::quat& q);
    void SetSize(const glm::vec3& size);

    void LookAt(const glm::vec3& point);


private:
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_size;

private:
    static glm::mat4 translate(glm::mat4&& model, const glm::vec3& vector);
    static glm::mat4 rotate(glm::mat4&& model, const glm::quat q);
    static glm::mat4 scale(glm::mat4&& model, const glm::vec3& vector);
};

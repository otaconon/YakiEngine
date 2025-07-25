#include "MovementSystem.h"

#include <print>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>

#include "../Ecs.h"
#include "../Components/Components.h"

MovementSystem::MovementSystem() = default;

void MovementSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();
    ecs.Each<Controller, Translation, Rotation, Scale>([dt](Hori::Entity, Controller& controller, Translation& t, Rotation& r, Scale& s) {
        glm::quat pitch = glm::angleAxis(controller.dy * dt, r.value * glm::normalize(glm::vec3{-1.f, 0.f, 0.f}));
        glm::quat yaw = glm::angleAxis(controller.dx * dt, glm::normalize(glm::vec3{0.f, 1.f, 0.f}));
        r.value = glm::normalize(yaw * pitch * r.value);

        glm::vec3 velocity = r.value * controller.direction * controller.speed * dt;
        t.value += velocity;
    });
}

#include "MovementSystem.h"

#include <print>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "../Ecs.h"
#include "../Components/Components.h"

MovementSystem::MovementSystem() = default;

void MovementSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();
    ecs.Each<Controller, Translation, Rotation, Scale>([dt](Hori::Entity, Controller& controller, Translation& t, Rotation& r, Scale& s) {
        r.pitch -= controller.dy * dt;
        r.yaw -= controller.dx * dt;

        r.pitch = glm::clamp(r.pitch, -glm::half_pi<float>(), glm::half_pi<float>());
        r.value = glm::quat(glm::vec3(r.pitch, r.yaw, r.roll));

        glm::vec3 velocity = r.value * controller.direction * controller.speed * dt;
        t.value += velocity;

        /*
        glm::quat pitch = glm::angleAxis(controller.dy * dt, r.value * glm::vec3{-1.f, 0.f, 0.f});
        glm::quat yaw = glm::angleAxis(controller.dx * dt, glm::vec3{0.f, 1.f, 0.f});
        r.value = glm::normalize(yaw * pitch * r.value); */
    });
}

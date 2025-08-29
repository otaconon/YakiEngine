#include "../../../include/YakiEngine/Core/Systems/MovementSystem.h"

#include <print>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "../../../include/YakiEngine/Core/Ecs.h"
#include "../../Components/Components.h"

MovementSystem::MovementSystem() = default;

void MovementSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();
    ecs.Each<Controller, Translation, Rotation, Scale>([dt](Hori::Entity, Controller& controller, Translation& t, Rotation& r, Scale& s) {
        float smoothFactor = 1.0f - exp(-30.0f * dt);
        controller.smoothedDx = glm::mix(controller.smoothedDx, controller.dx, smoothFactor);
        controller.smoothedDy = glm::mix(controller.smoothedDy, controller.dy, smoothFactor);

        r.pitch -= controller.smoothedDy * dt;
        r.yaw -= controller.smoothedDx * dt;
        r.pitch = glm::clamp(r.pitch, -glm::half_pi<float>(), glm::half_pi<float>());
        r.value = glm::quat(glm::vec3(r.pitch, r.yaw, r.roll));

        glm::vec3 velocity = r.value * controller.direction * controller.speed * dt;
        t.value += velocity;
    });
}

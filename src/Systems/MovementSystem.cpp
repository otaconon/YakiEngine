#include "MovementSystem.h"

#include <print>

#include "../Ecs.h"
#include "../Components/Controller.h"
#include "../Transform.h"


MovementSystem::MovementSystem() = default;

void MovementSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();
    ecs.Each<Controller, Transform>([dt](Hori::Entity, Controller& controller, Transform& transform) {
        glm::vec3 velocity = transform.GetRotation() * controller.direction * controller.speed * dt;
        transform.Translate(velocity);
        transform.Rotate(-transform.GetRight(), controller.dy * dt);
        transform.Rotate(transform.GetUp(), controller.dx * dt);
    });
}

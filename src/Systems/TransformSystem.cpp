#include "TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "../Ecs.h"
#include "../Components/Components.h"

TransformSystem::TransformSystem() = default;

void TransformSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ecs.Each<Translation, Rotation, Scale, LocalToParent>([](Hori::Entity, Translation& t, Rotation& r, Scale& s, LocalToParent& localToParent) {
        localToParent.value = glm::translate(glm::mat4(1.f), t.value) * glm::toMat4(r.value) * glm::scale(glm::mat4(1.f), s.value);
    });
}

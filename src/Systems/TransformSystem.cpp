#include "TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "../Ecs.h"
#include "../Components/Components.h"

TransformSystem::TransformSystem() = default;

void TransformSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ecs.Each<Translation, Rotation, Scale, LocalToParent>(
        [](Hori::Entity, Translation& t, Rotation& r, Scale& s, LocalToParent& localToParent) {
            localToParent.value = glm::translate(glm::mat4(1.f), t.value) *  glm::toMat4(r.value) * glm::scale(glm::mat4(1.f), s.value);
    });

    ecs.Each<LocalToWorld>([&](Hori::Entity entity, LocalToWorld&) {
       if (ecs.HasComponents<Parent>(entity)) {
           return;
       }

       updateHierarchy(entity, glm::mat4(1.0f));
   });
}

void TransformSystem::updateHierarchy(Hori::Entity entity, const glm::mat4& parentToWorld)
{
    auto& ecs = Ecs::GetInstance();

    auto localToWorld = ecs.GetComponent<LocalToWorld>(entity);

    glm::mat4 localMatrix = glm::mat4(1.0f);
    if (ecs.HasComponents<LocalToParent>(entity)) {
        auto localToParent = ecs.GetComponent<LocalToParent>(entity);
        localMatrix = localToParent->value;
    }

    localToWorld->value = parentToWorld * localMatrix;

    if (ecs.HasComponents<ParentToLocal>(entity)) {
        auto parentToLocal = ecs.GetComponent<ParentToLocal>(entity);
        parentToLocal->value = glm::inverse(localToWorld->value);
    }

    if (ecs.HasComponents<Children>(entity)) {
        auto children = ecs.GetComponent<Children>(entity);
        for (Hori::Entity child : children->value) {
            updateHierarchy(child, localToWorld->value);
        }
    }
}

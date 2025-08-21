#include "TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "../Ecs.h"
#include "../Components/Components.h"

TransformSystem::TransformSystem() = default;

void TransformSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ecs.ParallelEach<Translation, Rotation, Scale, LocalToWorld, Parent>(
        [&ecs](Hori::Entity e, Translation& t, Rotation& r, Scale& s, LocalToWorld& localToWorld, Parent& parent) {
            glm::mat4 newValue = glm::translate(glm::mat4(1.f), t.value) *  glm::toMat4(r.value) * glm::scale(glm::mat4(1.f), s.value);
            if (newValue == localToWorld.value)
                return;

            localToWorld.value = std::move(newValue);
            localToWorld.dirty = true;
            if (parent.value.Valid())
            {
                auto localToParent = ecs.GetComponent<LocalToParent>(e);
                localToParent->value = glm::translate(glm::mat4(1.f), t.value) *  glm::toMat4(r.value) * glm::scale(glm::mat4(1.f), s.value);
            }
    });

    ecs.Each<LocalToWorld, Children, Parent>([&](Hori::Entity entity, LocalToWorld& localToWorld, Children& children, Parent& parent) {
        if (parent.value.Valid())
            return;

        for (auto& e : children.value)
        {
            updateHierarchy(e, &localToWorld);
        }

        localToWorld.dirty = false;
   });
}

void TransformSystem::updateHierarchy(Hori::Entity entity, const LocalToWorld* parentToWorld)
{
    auto& ecs = Ecs::GetInstance();
    auto localToWorld = ecs.GetComponent<LocalToWorld>(entity);

    if (parentToWorld->dirty)
    {
        glm::mat4 localToParent = ecs.GetComponent<LocalToParent>(entity)->value;
        localToWorld->value = parentToWorld->value * localToParent;
    }

    /*
    if (ecs.HasComponents<ParentToLocal>(entity)) {
        auto parentToLocal = ecs.GetComponent<ParentToLocal>(entity);
        parentToLocal->value = glm::inverse(localToParent->value);
    }
    */

    auto children = ecs.GetComponent<Children>(entity);
    for (Hori::Entity child : children->value) {
        updateHierarchy(child, localToWorld);
    }

    localToWorld->dirty = false;
}

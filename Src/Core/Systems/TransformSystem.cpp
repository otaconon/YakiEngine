#include "Systems/TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Ecs.h"
#include "Components/CoreComponents.h"

TransformSystem::TransformSystem() = default;

void TransformSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    ecs.Each<DirtyTransform, Translation, Rotation, Scale, LocalToWorld, LocalToParent, Parent>(
        [&ecs](Hori::Entity e, DirtyTransform&, Translation& t, Rotation& r, Scale& s, LocalToWorld& localToWorld, LocalToParent& localToParent, Parent& parent) {
            if (parent.value.Valid())
            {
                r.value = glm::quat(glm::vec3(r.pitch, r.yaw, r.roll));
                localToParent.value = glm::translate(glm::mat4(1.f), t.value) *  glm::toMat4(r.value) * glm::scale(glm::mat4(1.f), s.value);
            }
            else
            {
                r.value = glm::quat(glm::vec3(r.pitch, r.yaw, r.roll));
                glm::mat4 newValue = glm::translate(glm::mat4(1.f), t.value) *  glm::toMat4(r.value) * glm::scale(glm::mat4(1.f), s.value);
                if (newValue == localToWorld.value)
                    return;

                localToWorld.value = std::move(newValue);
            }
    });

    ecs.Each<DirtyTransform, LocalToWorld, Children, Parent>([&](Hori::Entity entity, DirtyTransform&, LocalToWorld& localToWorld, Children& children, Parent& parent) {
        if (parent.value.Valid())
            return;

        for (auto& e : children.value)
        {
            updateHierarchy(e, &localToWorld);
        }

        ecs.RemoveComponents<DirtyTransform>(entity);
   });
}

void TransformSystem::updateHierarchy(Hori::Entity entity, const LocalToWorld* parentToWorld)
{
    auto& ecs = Ecs::GetInstance();
    auto localToWorld = ecs.GetComponent<LocalToWorld>(entity);

    glm::mat4 localToParent = ecs.GetComponent<LocalToParent>(entity)->value;
    localToWorld->value = parentToWorld->value * localToParent;

    auto children = ecs.GetComponent<Children>(entity);
    for (Hori::Entity child : children->value) {
        updateHierarchy(child, localToWorld);
    }
}

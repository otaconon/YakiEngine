#include "Systems/TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Ecs.h"
#include "Components/CoreComponents.h"

TransformSystem::TransformSystem() = default;

void TransformSystem::Update(float dt)
{
    auto& ecs = Ecs::GetInstance();

    // Kind of annoying, have to remember to not give one entity two move commands because first one will get discarded
    ecs.Each<MoveCommand, Translation>([&ecs](Hori::Entity e, MoveCommand& cmd, Translation& t){
        t.value += cmd.value;
        ecs.AddComponents(e, DirtyTransform{});
        ecs.RemoveComponents<MoveCommand>(e);
    });
    ecs.Each<RotateEulerCommand, Rotation>([&ecs](Hori::Entity e, RotateEulerCommand& cmd, Rotation& r){
        r.pitch += cmd.value.x;
        r.yaw += cmd.value.y;
        r.roll += cmd.value.z;
        ecs.AddComponents(e, DirtyTransform{});
        ecs.RemoveComponents<RotateEulerCommand>(e);
    });
    ecs.Each<ScaleCommand, Scale>([&ecs](Hori::Entity e, ScaleCommand& cmd, Scale& s){
        s.value += cmd.value;
        ecs.AddComponents(e, DirtyTransform{});
        ecs.RemoveComponents<ScaleCommand>(e);
    });

    ecs.ParallelEach<DirtyTransform, Translation, Rotation, Scale, LocalToWorld, LocalToParent, Parent>(
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

                localToWorld.value = newValue;
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

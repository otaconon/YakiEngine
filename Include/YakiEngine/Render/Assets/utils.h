#pragma once

#include "Assets/Mesh.h"
#include "Components/CoreComponents.h"

[[nodiscard]] inline Drawable create_drawable(std::shared_ptr<Mesh> mesh)
{
    Drawable drawable{mesh, {}, {}, {glm::mat4{1.f}, glm::mat4{}, glm::mat4{}}};
    return drawable;
}

inline void register_object(Hori::Entity e, std::shared_ptr<Mesh> mesh = nullptr, Translation pos = {})
{
    auto& ecs = Ecs::GetInstance();
    if (mesh)
        ecs.AddComponents(e, create_drawable(mesh));

    ecs.AddComponents(e, std::move(pos), Rotation{}, Scale{{1.f, 1.f, 1.f}}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, Children{}, Parent{});
}
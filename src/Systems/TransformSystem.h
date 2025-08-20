#pragma once

#include <World.h>
#include <glm/glm.hpp>

class TransformSystem : public Hori::System
{
public:
    TransformSystem();

    void Update(float dt) override;

private:
    void updateHierarchy(Hori::Entity entity, const glm::mat4& parentToWorld);
};
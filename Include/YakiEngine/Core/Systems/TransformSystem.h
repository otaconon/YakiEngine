#pragma once

#include <HECS/Core/World.h>
#include <glm/glm.hpp>
#include "Components/CoreComponents.h"

class TransformSystem : public Hori::System
{
public:
    TransformSystem();

    void Update(float dt) override;

private:
    void updateHierarchy(Hori::Entity entity, const LocalToWorld* parentToWorld);
};

#pragma once

#include <World.h>
#include <glm/glm.hpp>
#include "../../../../src/Components/Components.h"

class TransformSystem : public Hori::System
{
public:
    TransformSystem();

    void Update(float dt) override;

private:
    void updateHierarchy(Hori::Entity entity, const LocalToWorld* parentToWorld);
};

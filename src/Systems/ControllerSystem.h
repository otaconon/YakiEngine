#pragma once

#include "../Ecs.h"

class ControllerSystem : public Hori::System
{
public:
    ControllerSystem() = default;

    void Update(float dt) override;

};

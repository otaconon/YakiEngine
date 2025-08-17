#pragma once

#include <HECS/World.h>

class PhysicsSystem final : public Hori::System
{
public:
    PhysicsSystem();
    void Update(float dt) override;
};

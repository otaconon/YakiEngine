#pragma once

#include <World.h>

class PhysicsSystem final : public Hori::System
{
public:
    PhysicsSystem();
    void Update(float dt) override;

private:

};

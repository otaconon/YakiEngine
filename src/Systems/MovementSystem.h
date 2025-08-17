#pragma once

#include <HECS/World.h>

class MovementSystem : public Hori::System
{
public:
    MovementSystem();
    void Update(float dt) override;
};

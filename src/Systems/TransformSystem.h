#pragma once

#include <World.h>

class TransformSystem : public Hori::System
{
public:
    TransformSystem();

    void Update(float dt) override;

private:
};
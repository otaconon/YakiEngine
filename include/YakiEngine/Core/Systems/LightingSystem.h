#pragma once

#include <HECS/Core/System.h>

class LightingSystem final : public Hori::System
{
public:
    LightingSystem();
    void Update(float dt) override;
};

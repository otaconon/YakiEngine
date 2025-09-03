#pragma once

#include <HECS/Core/System.h>

#include "Vulkan/Renderer.h"

class LightingSystem final : public Hori::System
{
public:
    LightingSystem(Renderer* renderer);
    void Update(float dt) override;

private:
    Renderer* m_renderer;
};

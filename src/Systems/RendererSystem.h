#pragma once

#include <HECS/System.h>
#include <vector>

#include "../Components/Drawable.h"
#include "../Ecs.h"
#include "../Vulkan/Renderer.h"

class Renderer;

class RenderSystem : public Hori::System {
public:
    RenderSystem(Renderer& renderer);

    void Update(float dt) override;

private:
    Renderer* m_renderer;

    void renderDrawables();
    void renderGui();
};

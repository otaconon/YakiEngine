#pragma once

#include <System.h>
#include <vector>

#include "../Components/Drawable.h"
#include "../Ecs.h"
#include "../Vulkan/Renderer.h"

class Renderer;

class RenderSystem : public Hori::System {
public:
    explicit RenderSystem(Renderer& renderer);

    void Update(float dt) override;

private:
    Renderer* m_renderer;

    void renderDrawables(const glm::mat4& viewproj);
    void renderGui(float dt) const;
    void renderColliders();

    bool isVisible(const RenderObject& obj, const glm::mat4& viewproj);
};

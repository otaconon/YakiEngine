#pragma once

#include <bitset>
#include <System.h>
#include <vector>

#include "../Components/Drawable.h"
#include "../Ecs.h"
#include "../Vulkan/Renderer.h"

enum class ShowImGui : std::uint8_t
{
	PointLights,
    DirectionalLights
};

class Renderer;

class RenderSystem : public Hori::System {
public:
    explicit RenderSystem(Renderer& renderer);

    void Update(float dt) override;

private:
    Renderer* m_renderer;

    //std::bitset<sizeof(ShowImGui) * CHAR_BIT> m_showElements;
    std::bitset<8> m_showElements;
private:
    void renderDrawables(const glm::mat4& viewproj);
    void renderGui(float dt);
    void renderColliders();

    bool isVisible(const RenderObject& obj, const glm::mat4& viewproj);
};

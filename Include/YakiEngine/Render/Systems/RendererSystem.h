#pragma once

#include <bitset>
#include <HECS/Core/System.h>
#include <vector>
#include <glm/glm.hpp>

#include "Ecs.h"
#include "Vulkan/Renderer.h"
#include "Vulkan/VkTypes.h"

enum class ShowImGui : std::uint8_t {
  PointLights,
  DirectionalLights
};

class RenderSystem : public Hori::System {
public:
  explicit RenderSystem(Renderer *renderer);

  void Update(float dt) override;

private:
  Renderer *m_renderer;

  std::bitset<8> m_showElements;
  void renderDrawables(const glm::mat4 &viewproj);
  void renderDrawablesIndirect(const glm::mat4 &viewProj);
  void renderGui(float dt);

  bool isVisible(const RenderObject &obj, const glm::mat4 &viewproj);
};
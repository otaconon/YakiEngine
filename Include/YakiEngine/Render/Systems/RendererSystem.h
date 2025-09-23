#pragma once

#include <bitset>
#include <HECS/Core/System.h>
#include <vector>
#include <glm/glm.hpp>

#include "Ecs.h"
#include "Components/StaticObject.h"
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
  std::vector<IndirectBatch> m_indirectBatches;

  void renderStaticObjects(const glm::mat4 &viewProj);
  void renderGui(float dt);

  static RenderIndirectObjects sortObjects(RenderIndirectObjects &objects);
  static std::vector<IndirectBatch> packObjects(RenderIndirectObjects &objects);
};
#pragma once

#include "Assets/Mesh.h"
#include "Components/CoreComponents.h"

[[nodiscard]] inline Drawable create_drawable(std::shared_ptr<Mesh> mesh) {
  Drawable drawable{mesh, {glm::mat4{1.f}, glm::mat4{}, glm::mat4{}}};
  return drawable;
}

inline void register_object(Hori::Entity e, std::shared_ptr<Mesh> mesh = nullptr, Translation pos = {}) {
  auto &ecs = Ecs::GetInstance();
  if (mesh)
    ecs.AddComponents(e, create_drawable(mesh));

  ecs.AddComponents(e, std::move(pos), Rotation{}, Scale{{1.f, 1.f, 1.f}}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, Children{}, Parent{}, DirtyTransform{});
}

inline void init_default_data(VulkanContext& ctx, Swapchain& swapchain, DeletionQueue& deletionQueue) {
  auto& ecs = Ecs::GetInstance();

  DefaultData data {};

  uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
  uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
  std::array<uint32_t, 16 * 16> pixels{};
  for (int x = 0; x < 16; x++)
    for (int y = 0; y < 16; y++)
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
  data.errorTexture = std::make_shared<Texture>(&ctx, ctx.GetAllocator(), pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
  deletionQueue.PushFunction([&data] {
    data.errorTexture->Cleanup();
  });

  VkSamplerCreateInfo sampler = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_NEAREST,
    .minFilter = VK_FILTER_NEAREST
  };
  vkCreateSampler(ctx.GetDevice(), &sampler, nullptr, &data.samplerNearest);

  sampler = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR
  };
  vkCreateSampler(ctx.GetDevice(), &sampler, nullptr, &data.samplerLinear);

  deletionQueue.PushFunction([&ctx, &data]() {
    vkDestroySampler(ctx.GetDevice(), data.samplerNearest, nullptr);
    vkDestroySampler(ctx.GetDevice(), data.samplerLinear, nullptr);
  });

  // Initialize default shader passes
  auto vertShader = std::make_shared<Shader>(&ctx, "Shaders/Vertex/materials.vert.spv");
  auto fragShader = std::make_shared<Shader>(&ctx, "Shaders/Fragment/materials.frag.spv");
  auto effect = std::make_shared<ShaderEffect>(&ctx, vertShader, fragShader);
  auto forwardPass = std::make_shared<ShaderPass>(&ctx, swapchain, effect);
  auto shaderParams = std::make_shared<ShaderParameters>(glm::vec4{0.1f}, glm::vec4{0.1f}, glm::vec4{0.1f});
  auto effectTemplate = std::make_shared<EffectTemplate>();
  effectTemplate->passShaders[MeshPassType::Forward] = forwardPass;
  effectTemplate->defaultParameters = shaderParams;
  effectTemplate->transparency = TransparencyMode::Opaque;

  data.opaqueEffectTemplate = std::move(effectTemplate);

  ecs.AddSingletonComponent(std::move(data));
}
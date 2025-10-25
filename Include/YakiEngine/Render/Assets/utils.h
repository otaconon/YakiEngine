#pragma once

#include "Assets/Mesh.h"
#include "Components/CoreComponents.h"
#include "Components/DynamicObject.h"
#include "Components/StaticObject.h"

inline void register_dynamic_object(Hori::Entity e, DynamicObject object, Translation pos = {}) {
  auto &ecs = Ecs::GetInstance();
  ecs.AddComponents(e, std::move(object), std::move(pos));
  ecs.AddComponents(e, Rotation{}, Scale{{1.f, 1.f, 1.f}}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, Children{}, Parent{}, DirtyTransform{});
}

inline void register_static_object(Hori::Entity e, StaticObject object, Translation pos = {}) {
   auto &ecs = Ecs::GetInstance();
  ecs.AddComponents(e, std::move(object), std::move(pos), DirtyStaticObject{});
  ecs.AddComponents(e, Rotation{}, Scale{{1.f, 1.f, 1.f}}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, Children{}, Parent{}, DirtyTransform{});
}

inline void init_default_data(std::shared_ptr<VulkanContext> ctx, Swapchain& swapchain, DeletionQueue& deletionQueue) {
  auto& ecs = Ecs::GetInstance();

  DefaultData data {};

  uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
  uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
  std::array<uint32_t, 16 * 16> pixels{};
  for (int x = 0; x < 16; x++)
    for (int y = 0; y < 16; y++)
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
  data.errorTexture = std::make_shared<Texture>(ctx, pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

  VkSamplerCreateInfo sampler = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_NEAREST,
    .minFilter = VK_FILTER_NEAREST
  };
  vkCreateSampler(ctx->GetDevice(), &sampler, nullptr, &data.samplerNearest);

  sampler = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR
  };
  vkCreateSampler(ctx->GetDevice(), &sampler, nullptr, &data.samplerLinear);

  deletionQueue.PushFunction([ctx, &ecs]() {
    auto data = ecs.GetSingletonComponent<DefaultData>();
    vkDestroySampler(ctx->GetDevice(), data->samplerNearest, nullptr);
    vkDestroySampler(ctx->GetDevice(), data->samplerLinear, nullptr);
  });

  ecs.AddSingletonComponent(std::move(data));
}
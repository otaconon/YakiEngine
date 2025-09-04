#include "Assets/AssetMngr.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <stb_image.h>
#include <glm/gtx/quaternion.hpp>

#include "Ecs.h"
#include "Components/CoreComponents.h"
#include "Assets/AssetHandle.h"

// TODO: Uncomment this and move from here
/*
void AssetMngr::initDefaultTextures() {
  uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
  m_defaultTextures.whiteTexture = std::make_shared<Texture>(m_ctx, m_ctx->GetAllocator(), static_cast<void *>(&white), VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

  uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
  uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
  std::array<uint32_t, 16 * 16> pixels{};
  for (int x = 0; x < 16; x++)
    for (int y = 0; y < 16; y++)
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
  m_defaultTextures.errorTexture = std::make_shared<Texture>(m_ctx, m_ctx->GetAllocator(), pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
  m_deletionQueue.PushFunction([this] {
    m_defaultTextures.whiteTexture->Cleanup();
    m_defaultTextures.errorTexture->Cleanup();
  });

  VkSamplerCreateInfo sampler = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  sampler.magFilter = VK_FILTER_NEAREST;
  sampler.minFilter = VK_FILTER_NEAREST;
  vkCreateSampler(m_ctx->GetDevice(), &sampler, nullptr, &m_defaultTextures.samplerNearest);
  sampler.magFilter = VK_FILTER_LINEAR;
  sampler.minFilter = VK_FILTER_LINEAR;
  vkCreateSampler(m_ctx->GetDevice(), &sampler, nullptr, &m_defaultTextures.samplerLinear);

  m_deletionQueue.PushFunction([this]() {
    vkDestroySampler(m_ctx->GetDevice(), m_defaultTextures.samplerNearest, nullptr);
    vkDestroySampler(m_ctx->GetDevice(), m_defaultTextures.samplerLinear, nullptr);
  });
}
*/

std::shared_ptr<Asset> AssetMngr::getAssetImpl(AssetHandle handle) {
  auto it = m_registry.find(handle);
  if (it == m_registry.end()) {
    return nullptr;
  }
  return it->second;
}

AssetHandle AssetMngr::registerAssetImpl(std::shared_ptr<Asset> asset) {
  AssetHandle newHandle{m_currentHandleId++};
  m_registry[newHandle] = asset;
  return newHandle;
}
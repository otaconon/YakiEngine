#pragma once

#include <vulkan/vulkan.h>

#include "../../Core/Assets/Asset.h"
#include "Texture.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/Descriptors/DescriptorWriter.h"
#include "Vulkan/Descriptors/DescriptorLayoutBuilder.h"

struct MaterialResources {
  std::shared_ptr<Texture> colorImage;
  VkSampler colorSampler;

  std::shared_ptr<Texture> metalRoughImage;
  VkSampler metalRoughSampler;

  VkBuffer dataBuffer;
  uint32_t dataBufferOffset;
};

class MaterialBuilder {
public:
  explicit MaterialBuilder(VulkanContext *ctx);
  ~MaterialBuilder();

  void BuildPipelines(Swapchain &swapchain, VkDescriptorSetLayout gpuSceneDataDescriptorLayout);
  MaterialInstance WriteMaterial(MaterialPass pass, const MaterialResources &resources, DescriptorAllocator &descriptorAllocator);

private:
  VulkanContext *m_ctx;

  MaterialPipeline m_opaquePipeline{};
  MaterialPipeline m_transparentPipeline{};

  VkDescriptorSetLayout m_materialLayout{};
  DescriptorWriter m_writer;
};
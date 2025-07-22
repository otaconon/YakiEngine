#pragma once

#include <cassert>

#include "VkTypes.h"
#include "Image.h"
#include "Swapchain.h"
#include "Descriptors/DescriptorWriter.h"

struct MaterialConstants {
    glm::vec4 colorFactors;
    glm::vec4 metal_rough_factors;
    glm::vec4 extra[14];
};
static_assert(sizeof(MaterialConstants) == 256);

struct MaterialResources {
    std::shared_ptr<Image> colorImage;
    VkSampler colorSampler;
    std::shared_ptr<Image> metalRoughImage;
    VkSampler metalRoughSampler;
    VkBuffer dataBuffer;
    uint32_t dataBufferOffset;
};

class MetallicRoughnessMaterial {
public:
    void BuildPipelines(std::shared_ptr<VulkanContext> ctx, Swapchain& swapchain, VkDescriptorSetLayout gpuSceneDataDescriptorLayout);
    MaterialInstance WriteMaterial(std::shared_ptr<VulkanContext> ctx, MaterialPass pass, const MaterialResources& resources, DescriptorAllocator& descriptorAllocator);

private:
    MaterialPipeline m_opaquePipeline{};
    MaterialPipeline m_transparentPipeline{};

    VkDescriptorSetLayout m_materialLayout{};
    DescriptorWriter m_writer;
};

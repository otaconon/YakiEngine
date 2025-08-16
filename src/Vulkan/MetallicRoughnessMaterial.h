#pragma once

#include <cassert>

#include "VkTypes.h"
#include "../Assets/Texture.h"
#include "Swapchain.h"
#include "Descriptors/DescriptorWriter.h"

struct MaterialConstants {
    glm::vec4 colorFactors;
    glm::vec4 metal_rough_factors;
    glm::vec4 extra[14];
};
static_assert(sizeof(MaterialConstants) == 256);

struct MaterialResources {
    std::shared_ptr<Texture> colorImage;
    VkSampler colorSampler;
    std::shared_ptr<Texture> metalRoughImage;
    VkSampler metalRoughSampler;
    VkBuffer dataBuffer;
    uint32_t dataBufferOffset;
};

class MetallicRoughnessMaterial {
public:
    explicit MetallicRoughnessMaterial(VulkanContext* ctx);
    ~MetallicRoughnessMaterial();

    void BuildPipelines(Swapchain& swapchain, VkDescriptorSetLayout gpuSceneDataDescriptorLayout);
    MaterialInstance WriteMaterial(MaterialPass pass, const MaterialResources& resources, DescriptorAllocator& descriptorAllocator);

private:
    VulkanContext* m_ctx;

    MaterialPipeline m_opaquePipeline{};
    MaterialPipeline m_transparentPipeline{};

    VkDescriptorSetLayout m_materialLayout{};
    DescriptorWriter m_writer;
};

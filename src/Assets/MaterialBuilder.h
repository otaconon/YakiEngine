#pragma once

#include <vulkan/vulkan.h>

#include "Asset.h"
#include "../Vulkan/VulkanContext.h"
#include "../Vulkan/Swapchain.h"
#include "../Vulkan/Descriptors/DescriptorWriter.h"
#include "../Vulkan/Descriptors/DescriptorLayoutBuilder.h"

struct MaterialConstants {
    glm::vec4 colorFactors;
    glm::vec4 metalRoughtFactors;
    glm::vec4 specularColorFactors;
    glm::vec4 extra[13];
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

class MaterialBuilder
{
public:
	explicit MaterialBuilder(VulkanContext* ctx);
    ~MaterialBuilder();

    void BuildPipelines(Swapchain& swapchain, VkDescriptorSetLayout gpuSceneDataDescriptorLayout);
    MaterialInstance WriteMaterial(MaterialPass pass, const MaterialResources& resources, DescriptorAllocator& descriptorAllocator);

private:
	VulkanContext* m_ctx;

    MaterialPipeline m_opaquePipeline{};
    MaterialPipeline m_transparentPipeline{};

    VkDescriptorSetLayout m_materialLayout{};
    DescriptorWriter m_writer;
};
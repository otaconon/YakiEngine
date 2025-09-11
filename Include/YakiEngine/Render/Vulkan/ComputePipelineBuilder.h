#pragma once

#include "VulkanContext.h"

class ComputePipelineBuilder {
public:
    explicit ComputePipelineBuilder(std::shared_ptr<VulkanContext> ctx);

    VkPipeline CreatePipeline();

    [[nodiscard]] VkPipeline GetPipeline() const;
    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const;
    [[nodiscard]] DescriptorAllocator& GetDescriptorAllocator();

    void SetLayout(VkPipelineLayout layout);
    void SetShaders(VkShaderModule computeShader);

private:
    std::shared_ptr<VulkanContext> m_ctx;
    DescriptorAllocator m_descriptorAllocator{};

    VkPipeline m_pipeline{};
    VkPipelineShaderStageCreateInfo m_shaderStageCreateInfo{};
    VkPipelineLayout m_pipelineLayout{};
};

#pragma once

#include <array>

#include "Vulkan/VkInit.h"
#include "Vulkan/VulkanContext.h"

class PipelineBuilder
{
public:
    explicit PipelineBuilder(VulkanContext* ctx);

    VkPipeline CreatePipeline();
    VkPipeline CreateMRTPipeline(std::span<VkPipelineColorBlendAttachmentState> blendAttachments, std::span<VkFormat> colorFormats);

    [[nodiscard]] VkPipeline GetPipeline() const;
    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const;
    [[nodiscard]] DescriptorAllocator& GetDescriptorAllocator();

    void SetLayout(VkPipelineLayout layout);
    void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void SetMultisamplingNone();
    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthFormat(VkFormat format);

    void EnableDepthTest(bool depthWriteEnable);
    void EnableBlendingAdditive();
    void EnableBlendingAlphaBlend();

    void DisableBlending();
    void DisableDepthTest();

private:
    VulkanContext* m_ctx;
    DescriptorAllocator m_descriptorAllocator{};

    VkPipeline m_pipeline{};
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
    VkPipelineRasterizationStateCreateInfo m_rasterizer;
    VkPipelineMultisampleStateCreateInfo m_multisampling;
    VkPipelineDepthStencilStateCreateInfo m_depthStencil;
    VkPipelineRenderingCreateInfo m_renderInfo;
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
    VkFormat m_colorAttachmentFormat{};
    VkPipelineLayout m_pipelineLayout{};
};

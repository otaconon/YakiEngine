#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "Swapchain.h"
#include "VulkanContext.h"

#include "Descriptors/DescriptorAllocator.h"
#include "Descriptors/DescriptorLayoutBuilder.h"
#include "Descriptors/DescriptorWriter.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

class GraphicsPipeline {
public:
    GraphicsPipeline(const std::shared_ptr<VulkanContext>& ctx, Swapchain& swapchain);
    ~GraphicsPipeline();

    void CreateGraphicsPipeline();

    void SetLayout(VkPipelineLayout layout);
    void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void SetMultisamplingNone();
    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthFormat(VkFormat format);

    void EnableDepthTest();
    void EnableBlendingAdditive();
    void EnableBlendingAlphablend();

    void DisableBlending();
    void DisableDepthTest();

    [[nodiscard]] VkPipeline GetGraphicsPipeline() const;
    [[nodiscard]] VkPipelineLayout GetPipelineLayout() const;

private:
    std::shared_ptr<VulkanContext> m_ctx;

    VkPipeline m_graphicsPipeline{};
    DescriptorAllocator m_descriptorAllocator{};

    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
    VkPipelineRasterizationStateCreateInfo m_rasterizer;
    VkPipelineMultisampleStateCreateInfo m_multisampling;
    VkPipelineDepthStencilStateCreateInfo m_depthStencil;
    VkPipelineRenderingCreateInfo m_renderInfo;
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
    VkFormat m_colorAttachmentFormat{};
    VkPipelineLayout m_pipelineLayout{};

    VkDescriptorSet m_drawImageDescriptors{};
    VkDescriptorSetLayout m_drawImageDescriptorLayout{};

    DeletionQueue m_deletionQueue;

private:
    void initDescriptors(Swapchain& swapchain);
};
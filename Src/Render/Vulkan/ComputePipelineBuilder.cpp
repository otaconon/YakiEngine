#include "Vulkan/ComputePipelineBuilder.h"

#include "Vulkan/VkInit.h"

ComputePipelineBuilder::ComputePipelineBuilder(VulkanContext* ctx)
    : m_ctx{ctx}
{}

VkPipeline ComputePipelineBuilder::CreatePipeline()
{
    VkComputePipelineCreateInfo pipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = m_shaderStageCreateInfo,
        .layout = m_pipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE
    };

    if (vkCreateComputePipelines(m_ctx->GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline!");

    return m_pipeline;
}

VkPipeline ComputePipelineBuilder::GetPipeline() const { return m_pipeline; }
VkPipelineLayout ComputePipelineBuilder::GetPipelineLayout() const { return m_pipelineLayout; }
DescriptorAllocator& ComputePipelineBuilder::GetDescriptorAllocator() { return m_descriptorAllocator; }


void ComputePipelineBuilder::SetLayout(VkPipelineLayout layout)
{
    m_pipelineLayout = layout;
}

void ComputePipelineBuilder::SetShaders(VkShaderModule computeShader)
{
    m_shaderStageCreateInfo = VkInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, computeShader);
}

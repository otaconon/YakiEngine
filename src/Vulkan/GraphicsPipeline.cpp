#include "GraphicsPipeline.h"

#include <stdexcept>

#include "Swapchain.h"
#include "VkInit.h"
#include "VulkanContext.h"

GraphicsPipeline::GraphicsPipeline(const std::shared_ptr<VulkanContext>& ctx, Swapchain& swapchain)
    : m_ctx(ctx),
    m_inputAssembly{.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO},
    m_rasterizer{.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO},
    m_multisampling{.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO},
    m_depthStencil{.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO},
    m_renderInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO}
{
    initDescriptors(swapchain);
}

GraphicsPipeline::~GraphicsPipeline()
{
    m_deletionQueue.Flush();
}

void GraphicsPipeline::CreateGraphicsPipeline()
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    VkPipelineViewportStateCreateInfo viewportState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &m_colorBlendAttachment,
    };

    std::array dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &m_renderInfo,
        .stageCount = 2,
        .pStages = m_shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &m_inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &m_rasterizer,
        .pMultisampleState = &m_multisampling,
        .pDepthStencilState = &m_depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = m_pipelineLayout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };

    if (vkCreateGraphicsPipelines(m_ctx->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline!");

    m_deletionQueue.PushFunction([&] {
        vkDestroyPipeline(m_ctx->GetDevice(), m_graphicsPipeline, nullptr);
    });
}

void GraphicsPipeline::SetLayout(VkPipelineLayout layout) { m_pipelineLayout = layout; }

VkPipeline GraphicsPipeline::GetGraphicsPipeline() const { return m_graphicsPipeline; }
VkPipelineLayout GraphicsPipeline::GetPipelineLayout() const { return m_pipelineLayout; }

void GraphicsPipeline::initDescriptors(Swapchain& swapchain)
{
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    };

    m_descriptorAllocator.Init(m_ctx->GetDevice(), 10, sizes);

    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        m_drawImageDescriptorLayout = builder.Build(m_ctx->GetDevice(), VK_SHADER_STAGE_COMPUTE_BIT);
    }

    m_drawImageDescriptors = m_descriptorAllocator.Allocate(m_ctx->GetDevice(),m_drawImageDescriptorLayout);

    DescriptorWriter writer;
    writer.WriteImage(0, swapchain.GetDrawImage().view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.UpdateSet(m_ctx->GetDevice(), m_drawImageDescriptors);

    m_deletionQueue.PushFunction([&] {
        m_descriptorAllocator.DestroyPools(m_ctx->GetDevice());
        vkDestroyDescriptorSetLayout(m_ctx->GetDevice(), m_drawImageDescriptorLayout, nullptr);
    });


}

void GraphicsPipeline::SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
    m_shaderStages.clear();
    m_shaderStages.push_back(VkInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    m_shaderStages.push_back(VkInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void GraphicsPipeline::SetInputTopology(VkPrimitiveTopology topology)
{
    m_inputAssembly.topology = topology;
    m_inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void GraphicsPipeline::SetPolygonMode(VkPolygonMode mode)
{
    m_rasterizer.polygonMode = mode;
    m_rasterizer.lineWidth = 1.f;
}

void GraphicsPipeline::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    m_rasterizer.cullMode = cullMode;
    m_rasterizer.frontFace = frontFace;
}

void GraphicsPipeline::SetMultisamplingNone()
{
    m_multisampling.sampleShadingEnable = VK_FALSE;
    m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_multisampling.minSampleShading = 1.0f;
    m_multisampling.pSampleMask = nullptr;
    m_multisampling.alphaToCoverageEnable = VK_FALSE;
    m_multisampling.alphaToOneEnable = VK_FALSE;
}

void GraphicsPipeline::SetColorAttachmentFormat(VkFormat format)
{
    m_colorAttachmentFormat = format;
    m_renderInfo.colorAttachmentCount = 1;
    m_renderInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
}

void GraphicsPipeline::SetDepthFormat(VkFormat format)
{
    m_renderInfo.depthAttachmentFormat = format;
}

void GraphicsPipeline::EnableDepthTest()
{
    m_depthStencil.depthTestEnable = VK_TRUE;
    m_depthStencil.depthWriteEnable = VK_TRUE;
    m_depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    m_depthStencil.depthBoundsTestEnable = VK_FALSE;
    m_depthStencil.stencilTestEnable = VK_FALSE;
    m_depthStencil.front = {};
    m_depthStencil.back = {};
    m_depthStencil.minDepthBounds = 0.f;
    m_depthStencil.maxDepthBounds = 1.f;
}

void GraphicsPipeline::EnableBlendingAdditive()
{
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_TRUE;
    m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void GraphicsPipeline::EnableBlendingAlphablend()
{
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_TRUE;
    m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void GraphicsPipeline::DisableBlending()
{
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_FALSE;
}

void GraphicsPipeline::DisableDepthTest()
{
    m_depthStencil.depthTestEnable = VK_FALSE;
    m_depthStencil.depthWriteEnable = VK_FALSE;
    m_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    m_depthStencil.depthBoundsTestEnable = VK_FALSE;
    m_depthStencil.stencilTestEnable = VK_FALSE;
    m_depthStencil.front = {};
    m_depthStencil.back = {};
    m_depthStencil.minDepthBounds = 0.f;
    m_depthStencil.maxDepthBounds = 1.f;
}







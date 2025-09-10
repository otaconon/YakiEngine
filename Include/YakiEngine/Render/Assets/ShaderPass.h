#pragma once

#include "ShaderEffect.h"
#include "Vulkan/PipelineBuilder.h"
#include "Vulkan/Swapchain.h"

struct ShaderPass {
  std::shared_ptr<ShaderEffect> effect{nullptr};
  VkPipeline pipeline{VK_NULL_HANDLE};

  ShaderPass(VulkanContext *ctx, Swapchain &swapchain, std::shared_ptr<ShaderEffect> effect)
    : effect{effect},
      m_ctx{ctx} {
    PipelineBuilder pipelineBuilder(m_ctx);
    pipelineBuilder.SetShaders(effect->stages[ShaderStageType::Vertex].shader->module, effect->stages[ShaderStageType::Fragment].shader->module);
    pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.SetMultisamplingNone();
    pipelineBuilder.DisableBlending();
    pipelineBuilder.EnableDepthTest(true);
    pipelineBuilder.SetColorAttachmentFormat(swapchain.GetDrawImage().GetFormat());
    pipelineBuilder.SetDepthFormat(swapchain.GetDepthImage().GetFormat());
    pipelineBuilder.SetLayout(effect->pipelineLayout);

    std::array<VkPipelineColorBlendAttachmentState, 2> blendAttachments;
    blendAttachments[0] = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    blendAttachments[1] = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
    };

    std::array<VkFormat, 2> colorFormats{
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R32_UINT
    };

    pipeline = pipelineBuilder.CreateMRTPipeline(blendAttachments, colorFormats);
  }

private:
  VulkanContext *m_ctx;
};
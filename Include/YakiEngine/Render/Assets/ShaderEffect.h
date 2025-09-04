#pragma once

#include "Shader.h"
#include "Texture.h"

#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <spirv_cross/spirv_cross.hpp>

#include "Vulkan/VkUtils.h"
#include "vulkan/vulkan.h"
#include "Vulkan/VulkanContext.h"

enum class TransparencyMode {
  Opaque,
  Transparent
};

struct ShaderStage {
  Shader* shader;
  VkShaderStageFlagBits stage;
};

struct ShaderEffect {
  VkPipelineLayout pipelineLayout;
  std::array<VkDescriptorSetLayout, 4> descriptorSetLayouts;
  std::vector<ShaderStage> stages;

  ShaderEffect(VulkanContext *ctx, Shader *vertShader, Shader *fragShader)
    : m_ctx{ctx} {
    std::array<DescriptorLayoutBuilder, 4> builders;

    stages = {{vertShader, VK_SHADER_STAGE_VERTEX_BIT}, {fragShader, VK_SHADER_STAGE_FRAGMENT_BIT}};

    // TODO: Dont assume whats below
    // Assume that descriptor sets are shared for vertex and fragment shader
    for (const auto &[set, binding] : vertShader->ubos | std::views::keys) {
      builders[set].AddBinding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
    for (const auto &[set, binding] : vertShader->ssbos | std::views::keys) {
      builders[set].AddBinding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }
    for (const auto &[set, binding] : vertShader->sampledImages | std::views::keys) {
      builders[set].AddBinding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    for (const auto& [descSetLayouts, builder] : std::views::zip(descriptorSetLayouts, builders)) {
      descSetLayouts = builder.Build(m_ctx->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = VkInit::pipeline_layout_create_info();
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutCreateInfo.pPushConstantRanges = vertShader->pushConstantRanges.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = vertShader->pushConstantRanges.size();

    VK_CHECK(vkCreatePipelineLayout(m_ctx->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
  }

  ~ShaderEffect() {
      vkDestroyPipelineLayout(m_ctx->GetDevice(), pipelineLayout, nullptr);
      for (auto& descSetLayout : descriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(m_ctx->GetDevice(), descSetLayout, nullptr);
      }
  }

private:
  VulkanContext *m_ctx;
};
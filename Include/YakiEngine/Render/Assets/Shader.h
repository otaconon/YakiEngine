#pragma once

#include "../../../../build/release/_deps/spirvcross-src/spirv_cross.hpp"
#include "Vulkan/VkInit.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/Descriptors/DescriptorLayoutBuilder.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <vulkan/vulkan.h>

struct Shader {
  VkShaderModule module;
  VkPipelineLayout pipelineLayout;
  std::vector<VkDescriptorSetLayout> descSetLayouts;

  Shader(VulkanContext *ctx, const std::filesystem::path &path)
    : m_ctx{ctx} {
    if (!std::filesystem::exists(path)) {
      std::print("Failed to read file, path doesn't exist: {}", std::filesystem::absolute(path).string());
    }
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      std::print("Failed to open file: {}", std::filesystem::absolute(path).string());
    }

    size_t fileSize = file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .codeSize = buffer.size() * sizeof(uint32_t),
        .pCode = buffer.data()
    };

    if (vkCreateShaderModule(m_ctx->GetDevice(), &createInfo, nullptr, &module) != VK_SUCCESS) {
      std::print("Failed to create shader module from: {}", std::filesystem::absolute(path).string());
    }

    spirv_cross::Compiler compiler(buffer);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    std::map<uint32_t, DescriptorLayoutBuilder> sets;

    // Descriptor sets layout reflections
    for (const auto &ubo : resources.uniform_buffers) {
      uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
      uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
      sets[set].AddBinding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
      std::println("UBO: {} (set={}, binding={})", ubo.name, set, binding);
    }

    for (const auto &ssbo : resources.storage_buffers) {
      uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
      uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
      sets[set].AddBinding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
      std::println("SSBO: {} (set={}, binding={})", ssbo.name, set, binding);
    }

    for (const auto &sampledImage : resources.sampled_images) {
      uint32_t set = compiler.get_decoration(sampledImage.id, spv::DecorationDescriptorSet);
      uint32_t binding = compiler.get_decoration(sampledImage.id, spv::DecorationBinding);
      sets[set].AddBinding(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
      std::println("Texture2D: {} (set={}, binding={})", sampledImage.name, set, binding);
    }

    for (auto &builder : sets | std::views::values) {
      descSetLayouts.push_back(builder.Build(m_ctx->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    std::vector<VkPushConstantRange> pushConstantRanges;
    for (const auto &pc : resources.push_constant_buffers) {
      std::println("Push Constant: {}", pc.name);
      auto activeRanges = compiler.get_active_buffer_ranges(pc.id);
      for (const auto &[spirvRange, range] : std::views::zip(activeRanges, pushConstantRanges)) {
        range = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = static_cast<uint32_t>(spirvRange.offset),
            .size = static_cast<uint32_t>(spirvRange.range)
        };
      }
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = VkInit::pipeline_layout_create_info();
    pipelineLayoutCreateInfo.setLayoutCount = descSetLayouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = descSetLayouts.data();
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();

    VK_CHECK(vkCreatePipelineLayout(m_ctx->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
  }

private:
  VulkanContext *m_ctx;
};
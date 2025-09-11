#pragma once

#include <spirv_cross/spirv_cross.hpp>
#include "Vulkan/VulkanContext.h"
#include "Vulkan/Descriptors/DescriptorLayoutBuilder.h"

#include <filesystem>
#include <fstream>
#include <map>

struct Shader {
  VkShaderModule module;

  // Use std::map not std::unordered_map because it's better to have these collections sorted
  std::map<std::pair<uint32_t, uint32_t>, spirv_cross::Resource> ubos;
  std::map<std::pair<uint32_t, uint32_t>, spirv_cross::Resource> ssbos;
  std::map<std::pair<uint32_t, uint32_t>, spirv_cross::Resource> sampledImages;
  std::vector<VkPushConstantRange> pushConstantRanges;

  Shader(std::shared_ptr<VulkanContext> ctx, const std::filesystem::path &path)
    : m_ctx{ctx} {
    // Load shader
    if (!std::filesystem::exists(path)) {
      std::print("Failed to read file, path doesn't exist: {}", std::filesystem::absolute(path).string());
      return;
    }
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      std::print("Failed to open file: {}", std::filesystem::absolute(path).string());
      return;
    }

    size_t fileSize = file.tellg();
    size_t bufferSize = fileSize / sizeof(uint32_t);
    std::vector<uint32_t> buffer(bufferSize);

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
      return;
    }

    // Use reflections to fill shader info
    spirv_cross::Compiler compiler(std::move(buffer));
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    for (const auto &ubo : resources.uniform_buffers) {
      uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
      uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
      ubos[{set, binding}] = ubo;
      std::println("UBO: {} (set={}, binding={})", ubo.name, set, binding);
    }

    for (const auto &ssbo : resources.storage_buffers) {
      uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
      uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
      ssbos[{set, binding}] = ssbo;
      std::println("SSBO: {} (set={}, binding={})", ssbo.name, set, binding);
    }

    for (const auto &sampledImage : resources.sampled_images) {
      uint32_t set = compiler.get_decoration(sampledImage.id, spv::DecorationDescriptorSet);
      uint32_t binding = compiler.get_decoration(sampledImage.id, spv::DecorationBinding);
      sampledImages[{set, binding}] = sampledImage;
      std::println("Texture2D: {} (set={}, binding={})", sampledImage.name, set, binding);
    }

    for (const auto &pc : resources.push_constant_buffers) {
      auto type = compiler.get_type(pc.base_type_id);
      if (type.basetype == spirv_cross::SPIRType::Struct) {
        size_t totalSize = 0;
        for (uint32_t i = 0; i < type.member_types.size(); ++i) {
          auto memberOffset = compiler.type_struct_member_offset(type, i);
          auto memberSize = compiler.get_declared_struct_member_size(type, i);

          std::println("Member {}: offset={}, size={}", i, memberOffset, memberSize);
          totalSize = std::max(totalSize, memberOffset + memberSize);
        }
        pushConstantRanges.push_back(VkPushConstantRange{
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = static_cast<uint32_t>(totalSize)
        });
      }
    }
  }

  ~Shader() {
    vkDestroyShaderModule(m_ctx->GetDevice(), module, nullptr);
  }

private:
  std::shared_ptr<VulkanContext> m_ctx;
};
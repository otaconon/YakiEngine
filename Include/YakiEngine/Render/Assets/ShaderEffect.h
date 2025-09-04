#pragma once

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

enum class MeshPassType {
  Transparency,
  Forward
};

struct ShaderEffect {
  VulkanContext *ctx;
  VkPipelineLayout pipelineLayout;
  std::array<VkDescriptorSetLayout, 4> descriptorSetLayouts;

  struct ShaderStage {
    VkShaderModule shaderModule;
    VkShaderStageFlagBits stage;
  };

  std::vector<ShaderStage> stages;

  ShaderEffect(VulkanContext *ctx, std::filesystem::path vertPath, std::filesystem::path fragPath)
    : ctx{ctx} {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    if (!VkUtil::load_shader_module(vertPath, ctx->GetDevice(), &vertShader))
      std::println("Error when building the triangle vertex shader module");

    if (!VkUtil::load_shader_module(fragPath, ctx->GetDevice(), &fragShader))
      std::println("Error when building the triangle fragment shader module");

    stages.emplace_back(vertShader, VK_SHADER_STAGE_VERTEX_BIT);
    stages.emplace_back(fragShader, VK_SHADER_STAGE_FRAGMENT_BIT);



  }

  ~ShaderEffect() {
    for (auto &[shaderModule, stage] : stages) {
      vkDestroyShaderModule(ctx->GetDevice(), shaderModule, nullptr);
    }
  }
};

struct ShaderPass {
  ShaderEffect *effect{nullptr};
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout layout{VK_NULL_HANDLE};
};

struct ShaderParameters {
  glm::vec4 colorFactors;
  glm::vec4 metalRoughtFactors;
  glm::vec4 specularColorFactors;
  glm::vec4 extra[13];
};

struct EffectTemplate {
  std::vector<ShaderPass *> passShaders;

  ShaderParameters *defaultParameters;
  TransparencyMode transparency;
};

struct MaterialInfo {
  std::string baseEffect;
  std::unordered_map<std::string, std::string> textures; //name -> path
  std::unordered_map<std::string, std::string> customProperties;
  TransparencyMode transparency;
};

struct MaterialData {
  std::vector<Texture> textures;
  ShaderParameters *parameters;
  std::string baseTemplate;
};
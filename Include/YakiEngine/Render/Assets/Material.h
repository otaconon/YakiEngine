#pragma once

#include <Assets/AssetHandle.h>

#include "ShaderEffect.h"
#include "ShaderPass.h"
#include "EnumAccessArray.h"

struct ShaderParameters {
  glm::vec4 colorFactors;
  glm::vec4 metalRoughFactors;
  glm::vec4 specularColorFactors;
};

enum class MeshPassType : uint8_t {
  Transparency,
  Forward,
  Count
};

enum class TextureType : uint8_t {
  Color,
  MetalRough,
  Count
};

struct EffectTemplate {
  EnumAccessArray<std::shared_ptr<ShaderPass>, MeshPassType, static_cast<size_t>(MeshPassType::Count)> passShaders;
  std::shared_ptr<ShaderParameters> defaultParameters;
  TransparencyMode transparency;
};

struct Material {
  std::shared_ptr<EffectTemplate> original;
  EnumAccessArray<VkDescriptorSet, MeshPassType, static_cast<size_t>(MeshPassType::Count)> passSets;

  EnumAccessArray<AssetHandle<Texture>, TextureType, static_cast<size_t>(TextureType::Count)> textures;
  EnumAccessArray<VkSampler, TextureType, static_cast<size_t>(TextureType::Count)> samplers;

  ShaderParameters parameters;
};

struct MaterialInstance {
  ShaderParameters parameters;
  AssetHandle<Texture> textureHandle;
};


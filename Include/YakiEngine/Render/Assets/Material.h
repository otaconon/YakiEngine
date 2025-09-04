#pragma once

#include "ShaderEffect.h"
#include "ShaderPass.h"

struct ShaderParameters {
  glm::vec4 colorFactors;
  glm::vec4 metalRoughFactors;
  glm::vec4 specularColorFactors;
};

struct EffectTemplate {
  std::vector<ShaderPass *> passShaders;

  ShaderParameters *defaultParameters;
  TransparencyMode transparency;
};

struct Material {
  EffectTemplate *original;
  std::vector<VkDescriptorSet> passSets;

  std::vector<Texture> textures;

  ShaderParameters *parameters;
};

struct MaterialInfo {
  VkDescriptorSet materialSet;
  TransparencyMode transparency;
};

struct MaterialInstance {
  std::vector<Texture> textures;
  ShaderParameters* parameters;
  std::string baseTemplate;
};
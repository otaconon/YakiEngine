#pragma once

#include "ShaderEffect.h"

struct Material {
  EffectTemplate *original;
  std::vector<VkDescriptorSet> passSets;

  std::vector<Texture> textures;

  ShaderParameters *parameters;
};

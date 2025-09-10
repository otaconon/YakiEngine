#pragma once

#include "Assets/Material.h"
#include "Assets/Texture.h"

struct DefaultData {
  std::shared_ptr<Texture> errorTexture;
  VkSampler samplerNearest;
  VkSampler samplerLinear;
  std::shared_ptr<EffectTemplate> opaqueEffectTemplate;
};
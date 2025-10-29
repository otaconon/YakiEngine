#pragma once

#include "Assets/Material.h"
#include "Assets/Texture.h"

struct DefaultData {
  AssetHandle<Texture> errorTexture;
  VkSampler samplerNearest;
  VkSampler samplerLinear;
};
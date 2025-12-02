#pragma once

#include "Assets/Material.h"
#include "Assets/Texture.h"

struct DefaultData {
  AssetHandle<Texture> errorTexture;
  AssetHandle<Texture> whiteTexture;
  VkSampler samplerNearest;
  VkSampler samplerLinear;
};
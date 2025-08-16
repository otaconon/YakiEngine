#pragma once

#include "Assets/Texture.h"

struct DefaultData
{
    std::shared_ptr<Texture> whiteTexture;
    std::shared_ptr<Texture> errorTexture;
    VkSampler samplerNearest;
    VkSampler samplerLinear;
};
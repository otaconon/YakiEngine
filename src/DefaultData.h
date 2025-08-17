#pragma once

#include "Assets/Texture.h"

struct DefaultTextures
{
	std::shared_ptr<Texture> whiteTexture;
    std::shared_ptr<Texture> errorTexture;
    VkSampler samplerNearest;
    VkSampler samplerLinear;
};

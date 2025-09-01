#pragma once

#include "Vulkan/VkUtils.h"
#include "Vulkan/VkTypes.h"
#include "Assets/Mesh.h"


struct Drawable {
    std::shared_ptr<Mesh> mesh;
    UniformBufferObject ubo;
};





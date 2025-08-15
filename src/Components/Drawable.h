#pragma once

#include "../Vulkan/VkUtils.h"
#include "../Vulkan/VkTypes.h"
#include "../Assets/Mesh.h"


struct Drawable {
    std::shared_ptr<Mesh> mesh;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    UniformBufferObject ubo;
};





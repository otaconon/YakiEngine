#pragma once

#include <Entity.h>
#include <memory>
#include <string>
#include <unordered_map>

#include "../Vulkan/VkTypes.h"
#include "../Vulkan/Image.h"

struct GltfObject
{
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
    std::unordered_map<std::string, Hori::Entity> nodes;
    std::unordered_map<std::string, Image> images;
    std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> materials;

    std::vector<Hori::Entity> topNodes;
    std::vector<VkSampler> samplers;
    DescriptorAllocator descriptorPool;
    std::shared_ptr<Buffer> materialDataBuffer;

    VulkanContext* ctx;
};

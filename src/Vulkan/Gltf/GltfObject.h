#pragma once

#include <Entity.h>
#include <memory>
#include <string>
#include <unordered_map>

#include "../VkTypes.h"
#include "../Image.h"

struct GltfObject
{
    ~GltfObject()
    {
        for (auto& sampler : samplers)
        {
            vkDestroySampler(ctx->GetDevice(), sampler, nullptr);
        }

		descriptorPool.DestroyPools(ctx->GetDevice());
    }
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

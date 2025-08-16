#pragma once

#include <Entity.h>
#include <filesystem>
#include <fstream>
#include <print>

#include "DefaultData.h"
#include "Ecs.h"
#include "PropertyRegistry.h"
#include "Assets/Mesh.h"
#include "Components/Components.h"

inline std::vector<char> read_file(const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath))
    {
        std::print("Failed to read file, path {} doesn't exist", std::filesystem::absolute(filepath).string());
        return {};
    }

    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::print("Failed to read file, couldn't open path {}", std::filesystem::absolute(filepath).string());
        return {};
    }

    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

[[nodiscard]] inline Drawable create_drawable(std::shared_ptr<Mesh> mesh)
{
    Drawable drawable{mesh, {}, {}, {glm::mat4{1.f}, glm::mat4{}, glm::mat4{}}};
    return drawable;
}

inline void register_object(Hori::Entity e, std::shared_ptr<Mesh> mesh, Translation pos)
{
    auto& ecs = Ecs::GetInstance();
    ecs.AddComponents(e, create_drawable(mesh), std::move(pos), Rotation{}, Scale{{1.f, 1.f, 1.f}}, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, BoxCollider{{0.5f, 0.5f, 0.5f}, true});
    register_property<Translation>(e, "Translation");
    register_property<Rotation>(e, "Rotation");
    register_property<Scale>(e, "Scale");
}

[[nodiscard]] inline DefaultData create_default_data(VulkanContext* ctx, DeletionQueue& deletionQueue)
{
    DefaultData data;

    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    data.whiteTexture = std::make_shared<Texture>(ctx, ctx->GetAllocator(), static_cast<void*>(&white), VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16*16> pixels{};
    for (int x = 0; x < 16; x++)
        for (int y = 0; y < 16; y++)
            pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
    data.errorTexture = std::make_shared<Texture>(ctx, ctx->GetAllocator(), pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
    deletionQueue.PushFunction([&data] {
        data.whiteTexture->Cleanup();
        data.errorTexture->Cleanup();
    });

    VkSamplerCreateInfo sampler = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(ctx->GetDevice(), &sampler, nullptr, &data.samplerNearest);
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(ctx->GetDevice(), &sampler, nullptr, &data.samplerLinear);

    deletionQueue.PushFunction([&data, ctx]() {
        vkDestroySampler(ctx->GetDevice(), data.samplerNearest, nullptr);
        vkDestroySampler(ctx->GetDevice(), data.samplerLinear, nullptr);
    });

    return data;
}
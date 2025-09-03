#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <fastgltf/core.hpp>

#include "Vulkan/Buffer.h"
#include "Vulkan/VkInit.h"
#include "Vulkan/VulkanContext.h"
#include "../../Core/Assets/Asset.h"

class Texture final : public Asset
{
public:
    Texture();
    Texture(VulkanContext* ctx, VmaAllocator allocator, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
    Texture(VulkanContext* ctx, VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
    Texture(fastgltf::Asset& gltfAsset, fastgltf::Image& gltfImage);

    ~Texture() override;
    void Cleanup();

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    [[nodiscard]] VkImage GetImage() const;
    [[nodiscard]] VkImageView GetView() const;
    [[nodiscard]] VkExtent3D GetExtent() const;
    [[nodiscard]] VkFormat GetFormat() const;

private:
    VulkanContext* m_ctx;
    VmaAllocator m_allocator;
    VmaAllocation m_allocation{};

    VkImage m_image{};
    VkImageView m_view{};
    VkExtent3D m_extent{};
    VkFormat m_format{};
    uint32_t m_mipLevels;

    void createImage(VkImageUsageFlags usage);
    void generateMipMaps(VkCommandBuffer cmd);

    VkImageLayout getFinalLayout(VkFormat format, VkImageUsageFlags usage);
};

#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "../Vulkan/Buffer.h"
#include "../Vulkan/VkInit.h"
#include "../Vulkan/VulkanContext.h"
#include "Asset.h"

class Texture final : public Asset
{
public:
    Texture();
    Texture(VulkanContext* ctx, VmaAllocator allocator, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
    Texture(VulkanContext* ctx, VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);

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

    void createImage(VkImageUsageFlags usage, bool mipmapped);

    VkImageLayout getFinalLayout(VkFormat format, VkImageUsageFlags usage);
};

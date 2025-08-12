#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Buffer.h"
#include "VkInit.h"
#include "VulkanContext.h"

class Image
{
public:
    Image();
    Image(VulkanContext* ctx, VmaAllocator allocator, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
    Image(VulkanContext* ctx, VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);

    ~Image();
    void Cleanup();

    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

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

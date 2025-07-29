#include "Image.h"

Image::Image() = default;

Image::Image(VulkanContext* ctx, VmaAllocator allocator, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
    : m_ctx{ctx},
    m_allocator{allocator},
    m_extent{size},
    m_format{format}
{
    size_t data_size = size.depth * size.width * size.height * 4;
    Buffer uploadBuffer(m_allocator, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadBuffer.info.pMappedData, data, data_size);
    createImage(usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    m_ctx->ImmediateSubmit([&](VkCommandBuffer cmd) {
        VkUtil::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageExtent = size
        };

        vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        VkUtil::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
}

Image::Image(VulkanContext* ctx, VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
    : m_ctx{ctx},
    m_allocator{allocator},
    m_extent{size},
    m_format{format}
{
    createImage(usage, mipmapped);

    m_ctx->ImmediateSubmit([&](VkCommandBuffer cmd) {
        VkImageLayout finalLayout = getFinalLayout(format, usage);
        VkUtil::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_UNDEFINED, finalLayout);
    });
}

Image::~Image()
{
    Cleanup();
}

Image::Image(Image&& other) noexcept
    : m_ctx{std::move(other.m_ctx)},
    m_allocator{other.m_allocator},
    m_allocation{other.m_allocation},
    m_image{other.m_image},
    m_view{other.m_view},
    m_extent{other.m_extent},
    m_format{other.m_format}
{
    other.m_allocator = nullptr;
    other.m_allocation = nullptr;
    other.m_image = VK_NULL_HANDLE;
    other.m_view = VK_NULL_HANDLE;
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other) {
        Cleanup();

        m_ctx = std::move(other.m_ctx);
        m_allocator = other.m_allocator;
        m_allocation = other.m_allocation;
        m_image = other.m_image;
        m_view = other.m_view;
        m_extent = other.m_extent;
        m_format = other.m_format;

        other.m_allocator = nullptr;
        other.m_allocation = nullptr;
        other.m_image = VK_NULL_HANDLE;
        other.m_view = VK_NULL_HANDLE;
    }
    return *this;
}

VkImage Image::GetImage() const { return m_image; }
VkImageView Image::GetView() const { return m_view; }
VkExtent3D Image::GetExtent() const { return m_extent; }
VkFormat Image::GetFormat() const { return m_format; }

void Image::createImage(VkImageUsageFlags usage, bool mipmapped) {
    VkImageCreateInfo imgInfo = VkInit::image_create_info(m_format, usage, m_extent);
    if (mipmapped)
        imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_extent.width, m_extent.height)))) + 1;

    VmaAllocationCreateInfo allocInfo {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VK_CHECK(vmaCreateImage(m_allocator, &imgInfo, &allocInfo, &m_image, &m_allocation, nullptr));

    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (m_format == VK_FORMAT_D32_SFLOAT)
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkImageViewCreateInfo view_info = VkInit::imageview_create_info(m_format, m_image, aspectFlag);
    view_info.subresourceRange.levelCount = imgInfo.mipLevels;

    VK_CHECK(vkCreateImageView(m_ctx->GetDevice(), &view_info, nullptr, &m_view));
}

void Image::Cleanup()
{
    if (m_image != VK_NULL_HANDLE && m_view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_ctx->GetDevice(), m_view, nullptr);
        vmaDestroyImage(m_allocator, m_image, m_allocation);
    }
    m_view = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
}

VkImageLayout Image::getFinalLayout(VkFormat format, VkImageUsageFlags usage)
{
    if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM)
        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
        return VK_IMAGE_LAYOUT_GENERAL;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    return VK_IMAGE_LAYOUT_GENERAL;
}

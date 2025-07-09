#include "Swapchain.h"

#include <algorithm>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <ranges>
#include <SDL3/SDL_events.h>

Swapchain::Swapchain(const std::shared_ptr<VulkanContext>& ctx, SDL_Window* window)
    : m_ctx(ctx),
    m_window(window),
    m_extent{}
{
    VmaAllocatorCreateInfo allocatorInfo {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_ctx->GetPhysicalDevice(),
        .device = m_ctx->GetDevice(),
        .instance = m_ctx->GetInstance(),
    };
    vmaCreateAllocator(&allocatorInfo, &m_allocator);

    m_deletionQueue.PushFunction([&]() {
        vmaDestroyAllocator(m_allocator);
    });

    createSwapchain();
    createImageViews();
    createDrawImage();
    createDepthImage();
}

Swapchain::~Swapchain()
{
    cleanupSwapchain();
}

void Swapchain::RecreateSwapchain()
{
    while (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED) {
        SDL_Event event;
        SDL_WaitEvent(&event);
    }
    vkDeviceWaitIdle(m_ctx->GetDevice());

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createDrawImage();
}

VkSwapchainKHR& Swapchain::GetSwapchain() { return m_swapchain; }
VkFormat& Swapchain::GetImageFormat() { return m_format; }
VkExtent2D Swapchain::GetExtent() const { return m_extent; }
bool Swapchain::IsResized() const { return m_resized; }
VkImage Swapchain::GetImage(uint32_t idx) const { return m_images[idx]; }
Image Swapchain::GetDrawImage() const { return m_drawImage; }
Image Swapchain::GetDepthImage() const { return m_depthImage; }
VkImageView Swapchain::GetImageView(uint32_t idx) const { return m_imageViews[idx]; }
SwapChainSupportDetails Swapchain::GetSwapchainSupport() const { return m_swapchainSupport; }

void Swapchain::SetResized(bool resized)
{
    m_resized = resized;
}

void Swapchain::createSwapchain()
{
    m_swapchainSupport = VkUtil::query_swapchain_support(m_ctx->GetPhysicalDevice(), m_ctx->GetSurface());

    auto [format, colorSpace] = chooseSwapSurfaceFormat(m_swapchainSupport.formats);
    m_format = format;

    VkPresentModeKHR presentMode = chooseSwapPresentMode(m_swapchainSupport.presentModes);
    m_extent = chooseSwapExtent(m_swapchainSupport.capabilities, m_window);

    uint32_t imageCount = m_swapchainSupport.capabilities.minImageCount + 1;
    if (m_swapchainSupport.capabilities.maxImageCount > 0 && imageCount > m_swapchainSupport.capabilities.maxImageCount)
        imageCount = m_swapchainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_ctx->GetSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = m_format;
    createInfo.imageColorSpace = colorSpace;
    createInfo.imageExtent = m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = VkUtil::find_queue_families(m_ctx->GetPhysicalDevice(), m_ctx->GetSurface());
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = m_swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_ctx->GetDevice(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
        throw std::runtime_error("failed to create swap chain!");
}

void Swapchain::createImageViews()
{
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_ctx->GetDevice(), m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_ctx->GetDevice(), m_swapchain, &imageCount, m_images.data());

    // Create Swapchain image views
    m_imageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++)
    {
        m_imageViews[i] = createImageView(m_images[i], m_format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Swapchain::createDrawImage()
{
    VkExtent3D drawImageExtent = {m_extent.width, m_extent.height, 1};
    m_drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_drawImage.extent = drawImageExtent;

    VkImageUsageFlags drawImageUsages {
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    VkImageCreateInfo rimgInfo = VkInit::image_create_info(m_drawImage.format, drawImageUsages, drawImageExtent);
    VmaAllocationCreateInfo rimgAllocInfo {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    vmaCreateImage(m_allocator, &rimgInfo, &rimgAllocInfo, &m_drawImage.image, &m_drawImage.allocation, nullptr);
    VkImageViewCreateInfo rview_info = VkInit::imageview_create_info(m_drawImage.format, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(m_ctx->GetDevice(), &rview_info, nullptr, &m_drawImage.view));

    m_deletionQueue.PushFunction([=]() {
        vkDestroyImageView(m_ctx->GetDevice(), m_drawImage.view, nullptr);
        vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.allocation);
    });
}

void Swapchain::createDepthImage()
{
    m_depthImage.format = VK_FORMAT_D32_SFLOAT;
    m_depthImage.extent = m_drawImage.extent;
    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dimgInfo = VkInit::image_create_info(m_depthImage.format, depthImageUsages, m_drawImage.extent);
    VmaAllocationCreateInfo dimgAllocInfo {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    vmaCreateImage(m_allocator, &dimgInfo, &dimgAllocInfo, &m_depthImage.image, &m_depthImage.allocation, nullptr);

    //build an image-view for the draw image to use for rendering
    VkImageViewCreateInfo dview_info = VkInit::imageview_create_info(m_depthImage.format, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(m_ctx->GetDevice(), &dview_info, nullptr, &m_depthImage.view));

    m_deletionQueue.PushFunction([=]() {
        vkDestroyImageView(m_ctx->GetDevice(), m_depthImage.view, nullptr);
        vmaDestroyImage(m_allocator, m_depthImage.image, m_depthImage.allocation);
    });
}

VkImageView Swapchain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_ctx->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void Swapchain::cleanupSwapchain()
{
    VkDevice device = m_ctx->GetDevice();

    for (auto& imageView : m_imageViews)
        vkDestroyImageView(m_ctx->GetDevice(), imageView, nullptr);

    vkDestroySwapchainKHR(device, m_swapchain, nullptr);
    m_deletionQueue.Flush();
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

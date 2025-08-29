#include "Vulkan/Swapchain.h"

#include <algorithm>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <ranges>
#include <SDL3/SDL_events.h>

#include "Vulkan/VkInit.h"
#include "Vulkan/VkUtils.h"
#include "Vulkan/VulkanContext.h"

Swapchain::Swapchain(VulkanContext* ctx, SDL_Window* window)
    : m_ctx{ctx},
    m_window{window},
    m_renderScale{1.f},
    m_resized{false}
{
    VmaAllocatorCreateInfo allocatorInfo {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_ctx->GetPhysicalDevice(),
        .device = m_ctx->GetDevice(),
        .instance = m_ctx->GetInstance(),
    };
    vmaCreateAllocator(&allocatorInfo, &m_allocator);

    createSwapchain();
    createImageViews();
    createDrawImage();
    createDepthImage();
}

Swapchain::~Swapchain()
{
    cleanupSwapchain();
    vmaDestroyAllocator(m_allocator);
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
    createDepthImage();
}

VkSwapchainKHR& Swapchain::GetSwapchain() { return m_swapchain; }
VkFormat& Swapchain::GetImageFormat() { return m_format; }
VkExtent2D Swapchain::GetExtent() const { return m_extent; }
bool Swapchain::IsResized() const { return m_resized; }
float Swapchain::GetRenderScale() const { return m_renderScale; }
VkImage Swapchain::GetImage(uint32_t idx) const { return m_images[idx]; }
Texture& Swapchain::GetDrawImage() { return m_drawImage; }
Texture& Swapchain::GetDepthImage() { return m_depthImage; }
VkImageView Swapchain::GetImageView(uint32_t idx) const { return m_imageViews[idx]; }
SwapChainSupportDetails Swapchain::GetSwapchainSupport() const { return m_swapchainSupport; }

void Swapchain::SetResized(bool resized)
{
    m_resized = resized;
}

void Swapchain::createSwapchain()
{
    m_swapchainSupport = VkUtil::query_swapchain_support(m_ctx->GetPhysicalDevice(), m_ctx->GetSurface());

    auto [format, colorSpace] = VkUtil::chooseSwapSurfaceFormat(m_swapchainSupport.formats);
    m_format = format;

    VkPresentModeKHR presentMode = VkUtil::chooseSwapPresentMode(m_swapchainSupport.presentModes);
    m_extent = VkUtil::chooseSwapExtent(m_swapchainSupport.capabilities, m_window);

    uint32_t imageCount = m_swapchainSupport.capabilities.minImageCount + 1;
    if (m_swapchainSupport.capabilities.maxImageCount > 0 && imageCount > m_swapchainSupport.capabilities.maxImageCount)
        imageCount = m_swapchainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_ctx->GetSurface(),
        .minImageCount = imageCount,
        .imageFormat = m_format,
        .imageColorSpace = colorSpace,
        .imageExtent = m_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    };

    auto [graphicsFamily, presentFamily] = VkUtil::find_queue_families(m_ctx->GetPhysicalDevice(), m_ctx->GetSurface());
    uint32_t queueFamilyIndices[] = {graphicsFamily.value(), presentFamily.value()};

    if (graphicsFamily != presentFamily) {
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
    VkImageUsageFlags drawImageUsage {
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    m_drawImage = Texture(m_ctx, m_allocator, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsage, false);
    m_deletionQueue.PushFunction([this] {
        m_drawImage.Cleanup();
    });
}

void Swapchain::createDepthImage()
{
    VkImageUsageFlags depthImageUsage { VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT };
    m_depthImage = Texture(m_ctx, m_allocator, m_drawImage.GetExtent(), VK_FORMAT_D32_SFLOAT, depthImageUsage, false);
    m_deletionQueue.PushFunction([this] {
        m_depthImage.Cleanup();
    });
}

VkImageView Swapchain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const
{
    VkImageViewCreateInfo viewInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkImageView imageView;
    if (vkCreateImageView(m_ctx->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void Swapchain::cleanupSwapchain()
{
    for (auto& imageView : m_imageViews)
        vkDestroyImageView(m_ctx->GetDevice(), imageView, nullptr);

    vkDestroySwapchainKHR(m_ctx->GetDevice(), m_swapchain, nullptr);
    m_deletionQueue.Flush();
}
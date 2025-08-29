#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Vulkan/VulkanContext.h"
#include "Vulkan/VkUtils.h"
#include "Vulkan/VkTypes.h"
#include "../Assets/Texture.h"

class Swapchain {
public:
	Swapchain(VulkanContext* ctx, SDL_Window* window);
	~Swapchain();

	void RecreateSwapchain();

	[[nodiscard]] VkSwapchainKHR& GetSwapchain();
	[[nodiscard]] VkExtent2D GetExtent() const;
	[[nodiscard]] VkFormat& GetImageFormat();
	[[nodiscard]] VkImage GetImage(uint32_t idx) const;
	[[nodiscard]] Texture& GetDrawImage();
	[[nodiscard]] Texture& GetDepthImage();
	[[nodiscard]] VkImageView GetImageView(uint32_t idx) const;
	[[nodiscard]] bool IsResized() const;
	[[nodiscard]] float GetRenderScale() const;
	[[nodiscard]] SwapChainSupportDetails GetSwapchainSupport() const;

	void SetResized(bool resized);

	Swapchain(Swapchain&& other) = delete;
	Swapchain& operator=(Swapchain&& other) = delete;
	Swapchain(const Swapchain&) = delete;
	Swapchain& operator=(const Swapchain&) = delete;

private:
	VulkanContext* m_ctx;
	SDL_Window* m_window;

    VkSwapchainKHR m_swapchain{};
	SwapChainSupportDetails m_swapchainSupport;

    Texture m_drawImage{};
	Texture m_depthImage{};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;

    VkFormat m_format{};
    VkExtent2D m_extent{};
	float m_renderScale;
    bool m_resized;

	VmaAllocator m_allocator{};
	DeletionQueue m_deletionQueue;

private:
	void createSwapchain();
	void createImageViews();
	void createDrawImage();
	void createDepthImage();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;

	void cleanupSwapchain();
};

#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "VulkanContext.h"
#include "VkUtils.h"
#include "VkTypes.h"

class Swapchain {
public:
	Swapchain(const std::shared_ptr<VulkanContext>& ctx, SDL_Window* window);
	~Swapchain();

	void RecreateSwapchain();

	[[nodiscard]] VkSwapchainKHR& GetSwapchain();
	[[nodiscard]] VkFormat& GetImageFormat();
	[[nodiscard]] VkExtent2D GetExtent() const;
	[[nodiscard]] bool IsResized() const;
	[[nodiscard]] VkImage GetImage(uint32_t idx) const;
	[[nodiscard]] Image GetDrawImage() const;
	[[nodiscard]] VkImageView GetImageView(uint32_t idx) const;
	[[nodiscard]] SwapChainSupportDetails GetSwapchainSupport() const;

	void SetResized(bool resized);

	Swapchain(Swapchain&& other) = delete;
	Swapchain& operator=(Swapchain&& other) = delete;
	Swapchain(const Swapchain&) = delete;
	Swapchain& operator=(const Swapchain&) = delete;

private:
	std::shared_ptr<VulkanContext> m_ctx;
	SDL_Window* m_window;

    VkSwapchainKHR m_swapchain{};
	SwapChainSupportDetails m_swapchainSupport;

    Image m_drawImage;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;

    VkFormat m_format{};
    VkExtent2D m_extent;
    bool m_resized = false;

	VmaAllocator m_allocator;
	DeletionQueue m_deletionQueue;

private:
	void createSwapchain();
	void createImageViews();
	void createDrawImage();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void cleanupSwapchain();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);
};

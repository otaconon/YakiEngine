#pragma once
#include <filesystem>
#include <optional>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <iostream>
#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>

#include "VkCheck.h"

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

namespace VkUtil
{
    SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    uint32_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);

    void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
    void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

    bool load_shader_module(const std::filesystem::path& filePath, VkDevice device, VkShaderModule* outShaderModule);
}





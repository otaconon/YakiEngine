#pragma once
#include <filesystem>
#include <array>
#include <optional>
#include <print>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stb_image.h>

#include <iostream>
#include <cstdlib>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "VkInit.h"

#ifndef NDEBUG
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
             std::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)
#else
#   define VK_CHECK(call)  (void)(call)
#endif

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
    SwapChainSupportDetails query_swapchain_support(const VkPhysicalDevice device, const VkSurfaceKHR surface);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    uint32_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
    void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

    bool load_shader_module(const std::filesystem::path& filePath, VkDevice device, VkShaderModule* outShaderModule);
}





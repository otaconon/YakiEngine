#pragma once

#include <functional>
#include <vector>
#include <SDL3/SDL_video.h>

#include "VkTypes.h"

class VulkanContext {
public:
    VulkanContext(SDL_Window* window);
    ~VulkanContext();

    [[nodiscard]] VkInstance GetInstance() const;
    [[nodiscard]] VkDevice GetDevice() const;
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const;
    [[nodiscard]] VkSurfaceKHR GetSurface() const;
    [[nodiscard]] VmaAllocator GetAllocator() const;
    [[nodiscard]] VkQueue GetGraphicsQueue() const;
    [[nodiscard]] VkQueue GetPresentQueue() const;
    [[nodiscard]] VkPhysicalDeviceProperties GetGpuProperties() const;

    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;


private:
    VkInstance m_instance{};
    VkDevice m_device{};
    VkPhysicalDevice m_physicalDevice{};
    VkSurfaceKHR m_surface{};
    VmaAllocator m_allocator{};

    VkQueue m_graphicsQueue{};
    VkQueue m_presentQueue{};

    DeletionQueue m_deletionQueue;

    VkFence m_immFence{};
    VkCommandBuffer m_immCommandBuffer{};
    VkCommandPool m_immCommandPool{};

    VkPhysicalDeviceProperties m_gpuProperties{};
    VkDebugUtilsMessengerEXT m_debugMessenger{};

    void createInstance();
    void createLogicalDevice();
    void createSurface(SDL_Window* window);

    void setupDebugMessenger();

    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device) const;

    static bool checkDeviceExtensionsSupport(VkPhysicalDevice device);
    static bool checkValidationLayerSupport(std::vector<const char*>& validationLayers);
    static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageSeverityFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};

#include "VulkanContext.h"

#include <print>
#include <set>
#include <stdexcept>
#include <SDL3/SDL_vulkan.h>

#include "VkInit.h"
#include "VkUtils.h"

#ifdef NDEBUG
    constexpr bool g_enableValidationLayers = false;
#else
    constexpr bool g_enableValidationLayers = true;
#endif

std::vector g_validationLayers {"VK_LAYER_KHRONOS_validation"};
std::vector g_deviceExtensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME};

VulkanContext::VulkanContext(SDL_Window* window)
{
    createInstance();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();

    VmaAllocatorCreateInfo allocatorInfo {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_physicalDevice,
        .device = m_device,
        .instance = m_instance,
    };
    vmaCreateAllocator(&allocatorInfo, &m_allocator);

    m_deletionQueue.PushFunction([&] {
        vmaDestroyAllocator(m_allocator);
    });

	VkFenceCreateInfo fenceCreateInfo = VkInit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immFence));

	auto [graphicsFamily, presentFamily] = VkUtil::find_queue_families(m_physicalDevice, m_surface);
	VkCommandPoolCreateInfo commandPoolInfo = VkInit::command_pool_create_info(graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_immCommandPool));
    VkCommandBufferAllocateInfo cmdAllocInfo = VkInit::command_buffer_allocate_info(m_immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_immCommandBuffer));

    m_deletionQueue.PushFunction([this] {
        vkDestroyCommandPool(m_device, m_immCommandPool, nullptr);
        vkDestroyFence(m_device, m_immFence, nullptr);
    });

    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_gpuProperties);
}

VulkanContext::~VulkanContext()
{
    if (g_enableValidationLayers)
        destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

    m_deletionQueue.Flush();

    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

VkInstance VulkanContext::GetInstance() const { return m_instance; }
VkDevice VulkanContext::GetDevice() const { return m_device; }
VkPhysicalDevice VulkanContext::GetPhysicalDevice() const { return m_physicalDevice; }
VkSurfaceKHR VulkanContext::GetSurface() const { return m_surface; }
VmaAllocator VulkanContext::GetAllocator() const { return m_allocator; }
VkQueue VulkanContext::GetGraphicsQueue() const { return m_graphicsQueue; }
VkQueue VulkanContext::GetPresentQueue() const { return m_presentQueue; }
VkPhysicalDeviceProperties VulkanContext::GetGpuProperties() const { return m_gpuProperties; }

void VulkanContext::createInstance()
{
    // Setup validation layers
    if (g_enableValidationLayers && !checkValidationLayerSupport(g_validationLayers))
        throw std::runtime_error("Validation layers requested, but not available");

    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    // Initialize extensions
    uint32_t extCount;
    const char* const * instanceExtensions = SDL_Vulkan_GetInstanceExtensions(&extCount);
    std::vector extensions(instanceExtensions, instanceExtensions + extCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Initialize debug create info
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);

    const VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = g_enableValidationLayers ? &debugCreateInfo : nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size()),
        .ppEnabledLayerNames = g_validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("failed to create instance!");
}

void VulkanContext::createSurface(SDL_Window* window)
{
    if (!SDL_Vulkan_CreateSurface(window, m_instance, nullptr, &m_surface))
        throw std::runtime_error("failed to create surface");
}

void VulkanContext::createLogicalDevice()
{
    auto [graphicsFamily, presentFamily] = VkUtil::find_queue_families(m_physicalDevice, m_surface);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set uniqueQueueFamilies = {graphicsFamily.value(), presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures {
        .independentBlend = VK_TRUE,
        .fillModeNonSolid = VK_TRUE
    };

    VkPhysicalDeviceSynchronization2Features sync2Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = VK_TRUE
    };
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .pNext = &sync2Features,
        .dynamicRendering = VK_TRUE
    };
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &dynamicRenderingFeatures,
        .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentShaderBarycentricFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV,
        .pNext = &bufferDeviceAddressFeatures,
        .fragmentShaderBarycentric = VK_TRUE
    };

    const VkDeviceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &fragmentShaderBarycentricFeatures,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size()),
        .ppEnabledLayerNames = g_validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(g_deviceExtensions.size()),
        .ppEnabledExtensionNames = g_deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures
    };

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device!");

    vkGetDeviceQueue(m_device, graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, presentFamily.value(), 0, &m_presentQueue);
}
void VulkanContext::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find a suitable GPU!");
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices = VkUtil::find_queue_families(device, m_surface);
    bool extensionsSupported = checkDeviceExtensionsSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = VkUtil::query_swapchain_support(device, m_surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void VulkanContext::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const
{
	VK_CHECK(vkResetFences(m_device, 1, &m_immFence));
	VK_CHECK(vkResetCommandBuffer(m_immCommandBuffer, 0));

	VkCommandBuffer cmd = m_immCommandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = VkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	function(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdInfo = VkInit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = VkInit::submit_info(&cmdInfo, nullptr, nullptr);

	VK_CHECK(vkQueueSubmit2(GetGraphicsQueue(), 1, &submit, m_immFence));
	VK_CHECK(vkWaitForFences(GetDevice(), 1, &m_immFence, true, 9999999999));
}

bool VulkanContext::checkDeviceExtensionsSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(g_deviceExtensions.begin(), g_deviceExtensions.end());

    for (const auto& [extensionName, specVersion] : availableExtensions) {
        requiredExtensions.erase(extensionName);
    }

    return requiredExtensions.empty();
}

void VulkanContext::setupDebugMessenger()
{
    if constexpr (!g_enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug messenger!");
}

bool VulkanContext::checkValidationLayerSupport(std::vector<const char*>& validationLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto layerName: validationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties: availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

VkResult VulkanContext::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
void VulkanContext::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")); func != nullptr)
        func(instance, debugMessenger, pAllocator);
}
void VulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr
    };
}
VkBool32 VulkanContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageSeverityFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::print("validation layer: {}\n", pCallbackData->pMessage);
    return VK_FALSE;
}


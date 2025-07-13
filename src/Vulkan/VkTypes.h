#pragma once

#include <array>
#include <deque>
#include <functional>
#include <span>
#include <vulkan/vulkan_core.h>

#include <vk_mem_alloc.h>

#include "Buffer.h"
#include "Descriptors/DescriptorAllocator.h"

// TODO: Improve the deletion queue, so that for each vk type it holds separate collection
struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void PushFunction(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void Flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)();
        }

        deletors.clear();
    }
};

struct FrameData
{
    VkCommandPool commandPool{};
    VkCommandBuffer commandBuffer{};

    VkSemaphore swapchainSemaphore{}, renderSemaphore{};
    VkFence renderFence{};

    DeletionQueue deletionQueue;
    DescriptorAllocator frameDescriptors{};
};

struct Vertex
{
    glm::vec3 position;
    float _pad0;
    glm::vec3 normal;
    float _pad1;
    glm::vec3 color;
    float _pad2;
    glm::vec2 uv;
    float _pad3[2];

    Vertex()
        : position{}, _pad0{},
        normal{}, _pad1{},
        color{}, _pad2{},
        uv{}, _pad3{}
    {}

    Vertex(const glm::vec3& p, const glm::vec3 n, const glm::vec3& c, const glm::vec2& t)
        : position{p}, _pad0{0.f},
        normal{n}, _pad1{0.f},
        color{c}, _pad2{0.f},
        uv{t}, _pad3{0.0f, 0.0f}
    {}

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
        attributeDescriptions[0] = {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, position)
        };
        attributeDescriptions[1] = {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, normal)
        };
        attributeDescriptions[2] = {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, color)
        };
        attributeDescriptions[3] = {
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, uv)
        };

        return attributeDescriptions;
    }
};
static_assert(sizeof(Vertex) == 64);

struct GPUMeshBuffers
{
    Buffer vertexBuffer;
    Buffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

struct GPUDrawPushConstants {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    VkDeviceAddress vertexBuffer;
};

struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
};

struct Mesh {
    std::string name;

    std::vector<GeoSurface> surfaces;
    std::shared_ptr<GPUMeshBuffers> meshBuffers;
};
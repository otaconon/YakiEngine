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
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;

    Vertex()
        : position{},
        uv_x{},
        normal{},
        color{},
        uv_y{}
    {}

    Vertex(const glm::vec3& p, const glm::vec3 n, const glm::vec4& c, const glm::vec2& t)
        : position{p},
        uv_x{t.x},
        normal{n},
        color{c},
        uv_y{t.y}
    {}
};

struct GPUMeshBuffers
{
    Buffer vertexBuffer;
    Buffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

struct GPUDrawPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;

    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
};

enum class MaterialPass :uint8_t {
    MainColor,
    Transparent,
    Other
};

struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    std::shared_ptr<MaterialInstance> material;
};

struct Mesh {
    std::string name;

    std::vector<GeoSurface> surfaces;
    std::shared_ptr<GPUMeshBuffers> meshBuffers;
};

struct RenderObject {
    uint32_t indexCount;
    uint32_t firstIndex;
    VkBuffer indexBuffer;

    MaterialInstance* material;

    glm::mat4 transform;
    VkDeviceAddress vertexBufferAddress;
};

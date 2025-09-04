#pragma once

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <array>

#include "Buffer.h"
#include "Descriptors/DescriptorAllocator.h"
#include "DeletionQueue.h"
#include "Components/DirectionalLight.h"
#include "Components/PointLight.h"

struct FrameData {
  VkCommandPool commandPool{};
  VkCommandBuffer commandBuffer{};
  std::unique_ptr<Buffer> indirectDrawBuffer;

  VkSemaphore swapchainSemaphore{}, renderSemaphore{};
  VkFence renderFence{};

  DeletionQueue deletionQueue;
  DescriptorAllocator frameDescriptors{};
};

struct Vertex {
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
      uv_y{} {
  }

  Vertex(const glm::vec3 &p, const glm::vec3 n, const glm::vec4 &c, const glm::vec2 &t)
    : position{p},
      uv_x{t.x},
      normal{n},
      color{c},
      uv_y{t.y} {
  }
};

struct GPUMeshBuffers {
  Buffer vertexBuffer;
  Buffer indexBuffer;
  VkDeviceAddress vertexBufferAddress;
};

struct GPUDrawPushConstants {
  glm::mat4 worldMatrix;
  VkDeviceAddress vertexBuffer;
  uint32_t objectId;
};

struct GPUSceneData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 viewproj;
  glm::vec4 ambientColor;
  glm::vec4 eyePos;
};

static constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 10;
static constexpr uint32_t MAX_POINT_LIGHTS = 10;

struct GPULightData {
  uint32_t numDirectionalLights;
  uint32_t numPointLights;
  glm::uvec2 padding;
  std::array<DirectionalLight, MAX_DIRECTIONAL_LIGHTS> directionalLights;
  std::array<PointLight, MAX_POINT_LIGHTS> pointLights;
};

struct Bounds {
  glm::vec3 origin;
  float sphereRadius;
  glm::vec3 extents;
};

struct Material;
struct GeoSurface {
  uint32_t startIndex;
  uint32_t count;
  Bounds bounds;
  std::shared_ptr<Material> material;
};

struct ComputePipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};
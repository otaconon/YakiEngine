#pragma once

#include <array>

#include "Swapchain.h"
#include "VkTypes.h"
#include "VulkanContext.h"
#include "Components/Drawable.h"
#include "Components/DefaultData.h"
#include "RenderObject.h"

constexpr uint32_t FRAME_OVERLAP = 2;

struct RenderingStats {
  uint32_t triangleCount;
  uint32_t drawcallCount;
  float sceneUpdateTime;
};

struct PickingResources {
  std::shared_ptr<Texture> texture;
  std::shared_ptr<Buffer> stagingBuffer;
  uint32_t entityId;
};

struct IndirectBatch {
  glm::mat4 transform;
  Mesh *mesh;
  Material *material;
  uint32_t first;
  uint32_t count;
};

class Renderer {
public:
  Renderer(SDL_Window *window, std::shared_ptr<VulkanContext> ctx);
  ~Renderer();

  void BeginRendering();
  void Begin3DRendering();
  void RenderObjectsIndirect(std::span<RenderObject> objects);
  void RenderObjects(std::span<RenderObject> objects, std::span<size_t> order);
  void End3DRendering();
  void RenderImGui();
  void EndRendering();
  void WaitIdle();

  [[nodiscard]] Swapchain &GetSwapchain();
  [[nodiscard]] VkBuffer GetMaterialConstantsBuffer();
  [[nodiscard]] VkDescriptorSetLayout GetSceneDataDescriptorLayout();
  [[nodiscard]] uint32_t GetHoveredEntityId();
  [[nodiscard]] RenderingStats GetRenderingStats();
  [[nodiscard]] GPUSceneData &GetGpuSceneData();
  [[nodiscard]] GPULightData &GetGpuLightData();

private:
  SDL_Window *m_window;
  std::shared_ptr<VulkanContext> m_ctx;

  Swapchain m_swapchain;

  DeletionQueue m_deletionQueue;

  std::array<FrameData, 2> m_frames;
  uint32_t m_currentFrame;
  uint32_t m_currentImageIndex;

  DescriptorAllocator m_descriptorAllocator;

  VkDescriptorSetLayout m_drawImageDescriptorLayout{};
  VkDescriptorSetLayout m_singleImageDescriptorLayout{};
  VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout{};

  VkDescriptorSet m_globalDescriptor;
  VkDescriptorSet m_drawImageDescriptors{};

  GPUSceneData m_gpuSceneData;
  GPULightData m_gpuLightData;
  PickingResources m_pickingResources;
  RenderingStats m_stats;

  Buffer m_materialConstantsBuffer;

  void initCommands();
  void initImgui();
  void initSyncObjects();
  void initDescriptorAllocator();
  void initDescriptors();
  void initPicking();

  VkCommandBuffer beginSingleTimeCommands(VkCommandPool &commandPool) const;
  void endSingleTimeCommands(VkCommandPool &commandPool, VkCommandBuffer &commandBuffer) const;

  std::vector<IndirectBatch> packObjects(std::span<RenderObject> objects);

  FrameData &getCurrentFrame();
};
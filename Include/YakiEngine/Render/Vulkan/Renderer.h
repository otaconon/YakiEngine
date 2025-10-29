#pragma once

#include <array>

#include "Swapchain.h"
#include "VkTypes.h"
#include "VulkanContext.h"
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
  uint32_t indexCount, firstIndex;
  uint32_t firstInstance;
  uint32_t instanceCount;
  Mesh *mesh;
  MaterialInstance materialInstance;
};

class Renderer {
public:
  Renderer(SDL_Window *window, std::shared_ptr<VulkanContext> ctx);
  ~Renderer();

  void BeginRendering();
  void Begin3DRendering();
  void RenderStaticObjects(std::vector<IndirectBatch>& batches);
  void End3DRendering();
  void RenderImGui();
  void EndRendering();
  void WaitIdle();

  void UploadTextures(std::vector<std::shared_ptr<Texture>> &textures);
  void UpdateStaticObjects(RenderIndirectObjects& objects);

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

  std::array<FrameData, FRAME_OVERLAP> m_frames;
  uint32_t m_currentFrame;
  uint32_t m_currentImageIndex;

  DescriptorAllocator m_descriptorAllocator;

  // TODO: Move those from here
  std::unique_ptr<Buffer> m_objectIdsBuffer;
  std::unique_ptr<Buffer> m_transformsBuffer;
  std::unique_ptr<Buffer> m_paramsBuffer;
  std::unique_ptr<Buffer> m_textureIdsBuffer;
  std::unique_ptr<Buffer> m_samplersBuffer;

  VkDescriptorSetLayout m_drawImageDescriptorLayout{};
  VkDescriptorSetLayout m_singleImageDescriptorLayout{};
  VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout{};

  VkDescriptorSet m_frameDescriptor;
  VkDescriptorSet m_drawImageDescriptors{};
  VkDescriptorSet m_materialDataDescriptorSet{};
  std::shared_ptr<EffectTemplate> m_opaqueEffectTemplate{};

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

  FrameData &getCurrentFrame();
};
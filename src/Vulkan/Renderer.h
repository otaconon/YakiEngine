#pragma once

#include <array>

#include "Swapchain.h"
#include "VkTypes.h"
#include "VulkanContext.h"
#include "../Components/Drawable.h"
#include "../DefaultData.h"
#include "../Assets/MaterialBuilder.h"

constexpr uint32_t FRAME_OVERLAP = 2;

struct RenderingStats
{
    uint32_t triangleCount;
    uint32_t drawcallCount;
    float sceneUpdateTime;
};

struct PickingResources
{
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Buffer> stagingBuffer;
    uint32_t entityId;
};

class Renderer {
public:
    Renderer(SDL_Window* window, VulkanContext* ctx);
    ~Renderer();

    void BeginRendering();
    void Begin3DRendering();
    void RenderObjects(std::span<RenderObject> objects, std::span<size_t> order);
    void RenderWireframes(std::vector<WireframeObject>& objects);
    void End3DRendering();
    void RenderImGui();
    void EndRendering();
    void WaitIdle();
    
    [[nodiscard]] Swapchain& GetSwapchain();
    [[nodiscard]] VkBuffer GetMaterialConstantsBuffer();
    [[nodiscard]] VkDescriptorSetLayout GetSceneDataDescriptorLayout();
    [[nodiscard]] uint32_t GetHoveredEntityId();
    [[nodiscard]] RenderingStats GetRenderingStats();

    void WriteMaterialConstants(MaterialConstants& materialConstants);

private:
    SDL_Window* m_window;
	VulkanContext* m_ctx;

    Swapchain m_swapchain;

    VkDescriptorSetLayout m_wireframeDescriptorSetLayout;
    VkPipelineLayout m_wireframePipelineLayout;
    VkPipeline m_wireframePipeline;

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

    PickingResources m_pickingResources;
    RenderingStats m_stats;

    Buffer m_materialConstantsBuffer;

private:
    void initCommands();
    void initImgui();
    void initSyncObjects();
    void initDescriptorAllocator();
    void initDescriptors();
    void initWireframePipeline();
    void initPicking();

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool& commandPool) const;
    void endSingleTimeCommands(VkCommandPool& commandPool, VkCommandBuffer& commandBuffer) const;

    FrameData& getCurrentFrame();

    static VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
};

#pragma once

#include <array>

#include "MetallicRoughnessMaterial.h"
#include "Swapchain.h"
#include "VkTypes.h"
#include "VulkanContext.h"
#include "../Components/Drawable.h"

constexpr uint32_t FRAME_OVERLAP = 2;

class Renderer {
public:
    Renderer(SDL_Window* window, VulkanContext* ctx);
    ~Renderer();

    void BeginRendering();
    void RenderObjects(std::vector<RenderObject>& objects);
    void RenderImGui();
    void EndRendering();
    void WaitIdle();

private:
    SDL_Window* m_window;
	VulkanContext* m_ctx;

    Swapchain m_swapchain;

    DeletionQueue m_deletionQueue;

    std::array<FrameData, 2> m_frames;
    uint32_t m_currentFrame;
    uint32_t m_currentImageIndex;

    DescriptorAllocator m_descriptorAllocator;
    VkDescriptorSet m_drawImageDescriptors{};

    VkDescriptorSetLayout m_drawImageDescriptorLayout{};
    VkDescriptorSetLayout m_singleImageDescriptorLayout{};
    VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout{};

    std::shared_ptr<Image> m_whiteImage{};
    std::shared_ptr<Image> m_errorImage{};

    VkSampler m_defaultSamplerLinear{};
    VkSampler m_defaultSamplerNearest{};

    MaterialInstance m_defaultData{};
    MetallicRoughnessMaterial m_metalRoughMaterial;

private:
    void initCommands();
    void initImgui();
    void initSyncObjects();
    void initGraphicsPipeline();
    void initDescriptorAllocator();
    void initDescriptors();
    void initSamplers();
    void initDefaultTextures();
    void initDefaultData();

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool& commandPool) const;
    void endSingleTimeCommands(VkCommandPool& commandPool, VkCommandBuffer& commandBuffer) const;

    FrameData& getCurrentFrame();

    static VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
};

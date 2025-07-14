#pragma once

#include "GraphicsPipeline.h"
#include "Swapchain.h"
#include "VkTypes.h"
#include "VulkanContext.h"
#include "../Components/Drawable.h"

constexpr uint32_t FRAME_OVERLAP = 2;

class Renderer {
public:
    Renderer(SDL_Window* window, const std::shared_ptr<VulkanContext>& ctx);
    ~Renderer();

    void DrawFrame(std::vector<Drawable>& drawables);

    [[nodiscard]] std::shared_ptr<GPUMeshBuffers> UploadMesh(const std::span<uint32_t> indices, const std::span<Vertex> vertices) const;
    [[nodiscard]] std::optional<std::vector<std::shared_ptr<Mesh>>> LoadGltfMeshes(const std::filesystem::path& filePath) const;

private:
    SDL_Window* m_window;

	std::shared_ptr<VulkanContext> m_ctx;
    Swapchain m_swapchain;

    VmaAllocator m_allocator{};
    DeletionQueue m_deletionQueue;

    VkPipelineLayout m_graphicsPipelineLayout{};
    GraphicsPipeline m_graphicsPipeline;

    std::array<FrameData, 2> m_frames;
    uint32_t m_currentFrame;

    Image m_errorTexture{};
    VkSampler m_defaultSamplerLinear;
    VkSampler m_defaultSamplerNearest;
    VkDescriptorSetLayout m_singleImageDescriptorLayout;

    GPUSceneData m_sceneData;
    VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout; //TODO: Doesnt this belong to graphics pipeline?

private:
    void initCommands();
    void initImgui();
    void initSyncObjects();
    void initGraphicsPipeline();

    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, std::vector<Drawable>& drawables);

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool& commandPool) const;
    void endSingleTimeCommands(VkCommandPool& commandPool, VkCommandBuffer& commandBuffer) const;

    void drawImgui(VkCommandBuffer cmd, VkImageView targetImageView) const;
    void drawObjects(VkCommandBuffer cmd, std::vector<Drawable>& drawables);

    FrameData& getCurrentFrame();

    static VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
};

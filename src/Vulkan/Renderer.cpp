#include "Renderer.h"

#include <imgui.h>
#include <stdexcept>
#include <SDL3/SDL_vulkan.h>
#include <ranges>

#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>

#include "ImGuiStyles.h"
#include "VkInit.h"
#include "../Ecs.h"
#include "Descriptors/DescriptorLayoutBuilder.h"

Renderer::Renderer(SDL_Window* window, const std::shared_ptr<VulkanContext>& ctx)
    : m_window(window),
    m_ctx(ctx),
    m_swapchain(ctx, window),
    m_currentFrame(0),
	m_metalRoughMaterial(ctx)
{
	VmaAllocatorCreateInfo allocatorInfo {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = m_ctx->GetPhysicalDevice(),
		.device = m_ctx->GetDevice(),
		.instance = m_ctx->GetInstance(),
	};
	vmaCreateAllocator(&allocatorInfo, &m_allocator);

	m_deletionQueue.PushFunction([&] {
		vmaDestroyAllocator(m_allocator);
	});

	initCommands();
	initSyncObjects();
	initImgui();
	initDescriptorAllocator();
	initDescriptors();
	initSamplers();
	initDefaultTextures();
	initGraphicsPipeline();
	initDefaultData();
}

Renderer::~Renderer()
{
    VkDevice device = m_ctx->GetDevice();
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < FRAME_OVERLAP; i++)
    {
		vkDestroyCommandPool(device, m_frames[i].commandPool, nullptr);
		vkDestroyFence(device, m_frames[i].renderFence, nullptr);
		vkDestroySemaphore(device, m_frames[i].renderSemaphore, nullptr);
		vkDestroySemaphore(device, m_frames[i].swapchainSemaphore, nullptr);

    	m_frames[i].deletionQueue.Flush();
    }

	m_deletionQueue.Flush();
}

void Renderer::initCommands()
{
	auto [graphicsFamily, presentFamily] = VkUtil::find_queue_families(m_ctx->GetPhysicalDevice(), m_ctx->GetSurface());
	VkCommandPoolCreateInfo commandPoolInfo = VkInit::command_pool_create_info(graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(m_ctx->GetDevice(), &commandPoolInfo, nullptr, &m_frames[i].commandPool));
		VkCommandBufferAllocateInfo cmdAllocInfo = VkInit::command_buffer_allocate_info(m_frames[i].commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(m_ctx->GetDevice(), &cmdAllocInfo, &m_frames[i].commandBuffer));
	}
}

void Renderer::initImgui()
{
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes)),
		.pPoolSizes = pool_sizes,
	};

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(m_ctx->GetDevice(), &pool_info, nullptr, &imguiPool));

	ImGui::CreateContext();

	ImGui_ImplSDL3_InitForVulkan(m_window);

	ImGui_ImplVulkan_InitInfo init_info {
		.Instance = m_ctx->GetInstance(),
		.PhysicalDevice = m_ctx->GetPhysicalDevice(),
		.Device = m_ctx->GetDevice(),
		.Queue = m_ctx->GetGraphicsQueue(),
		.DescriptorPool = imguiPool,
		.MinImageCount = 3,
		.ImageCount = 3,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.UseDynamicRendering = true,
		.PipelineRenderingCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &m_swapchain.GetImageFormat(),
		},
	};

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();
	ImGui::UseCustomStyle();

	m_deletionQueue.PushFunction([this, imguiPool] {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(m_ctx->GetDevice(), imguiPool, nullptr);
	});
}

void Renderer::initSyncObjects()
{
	VkFenceCreateInfo fenceCreateInfo = VkInit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = VkInit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(m_ctx->GetDevice(), &fenceCreateInfo, nullptr, &m_frames[i].renderFence));
		VK_CHECK(vkCreateSemaphore(m_ctx->GetDevice(), &semaphoreCreateInfo, nullptr, &m_frames[i].swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(m_ctx->GetDevice(), &semaphoreCreateInfo, nullptr, &m_frames[i].renderSemaphore));
	}
}

void Renderer::initGraphicsPipeline()
{
	m_metalRoughMaterial.BuildPipelines(m_swapchain, m_gpuSceneDataDescriptorLayout);
}

void Renderer::initDescriptorAllocator()
{
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes {
	        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	m_descriptorAllocator.Init(m_ctx->GetDevice(), 10, sizes);

	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_drawImageDescriptorLayout = builder.Build(m_ctx->GetDevice(), VK_SHADER_STAGE_COMPUTE_BIT);
	}

	m_drawImageDescriptors = m_descriptorAllocator.Allocate(m_ctx->GetDevice(),m_drawImageDescriptorLayout);

	DescriptorWriter writer;
	writer.WriteImage(0, m_swapchain.GetDrawImage().GetView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.UpdateSet(m_ctx->GetDevice(), m_drawImageDescriptors);

	m_deletionQueue.PushFunction([&] {
		m_descriptorAllocator.DestroyPools(m_ctx->GetDevice());
		vkDestroyDescriptorSetLayout(m_ctx->GetDevice(), m_drawImageDescriptorLayout, nullptr);
	});
}

void Renderer::initDescriptors()
{
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_singleImageDescriptorLayout = builder.Build(m_ctx->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT);
		m_deletionQueue.PushFunction([this] {
			vkDestroyDescriptorSetLayout(m_ctx->GetDevice(), m_singleImageDescriptorLayout, nullptr);
		});
	}

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		std::vector<DescriptorAllocator::PoolSizeRatio> frame_sizes = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		m_frames[i].frameDescriptors.Init(m_ctx->GetDevice(), 1000, frame_sizes);

		m_deletionQueue.PushFunction([&, i]() {
			m_frames[i].frameDescriptors.DestroyPools(m_ctx->GetDevice());
		});
	}

	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_gpuSceneDataDescriptorLayout = builder.Build(m_ctx->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		m_deletionQueue.PushFunction([&] {
			vkDestroyDescriptorSetLayout(m_ctx->GetDevice(), m_gpuSceneDataDescriptorLayout, nullptr);
		});
	}
}

void Renderer::initSamplers()
{
	VkSamplerCreateInfo sampler = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;
	vkCreateSampler(m_ctx->GetDevice(), &sampler, nullptr, &m_defaultSamplerNearest);
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(m_ctx->GetDevice(), &sampler, nullptr, &m_defaultSamplerLinear);

	m_deletionQueue.PushFunction([&]() {
		vkDestroySampler(m_ctx->GetDevice(), m_defaultSamplerNearest,nullptr);
		vkDestroySampler(m_ctx->GetDevice(), m_defaultSamplerLinear,nullptr);
	});
}

void Renderer::initDefaultTextures()
{
	uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16*16 > pixels;
	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++)
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;

	m_errorTexture = std::make_shared<Image>(m_ctx, m_allocator, pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
	m_deletionQueue.PushFunction([this] {
		m_errorTexture->Cleanup();
	});
}

void Renderer::initDefaultData()
{
	Buffer materialConstants(m_allocator, sizeof(MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	MaterialConstants* sceneUniformData = static_cast<MaterialConstants*>(materialConstants.allocation->GetMappedData());
	sceneUniformData->colorFactors = glm::vec4{1,1,1,1};
	sceneUniformData->metal_rough_factors = glm::vec4{1,0.5,0,0};

	MaterialResources materialResources {
		.colorImage = m_errorTexture,
		.colorSampler = m_defaultSamplerLinear,
		.metalRoughImage = m_errorTexture,
		.metalRoughSampler = m_defaultSamplerLinear,
		.dataBuffer = materialConstants.buffer,
		.dataBufferOffset = 0
	};

	// TODO: Change this
	m_deletionQueue.PushFunction([buffer = std::make_shared<Buffer>(std::move(materialConstants))]() mutable {
		buffer->Cleanup();
	});

	m_defaultData = m_metalRoughMaterial.WriteMaterial(MaterialPass::MainColor, materialResources, m_descriptorAllocator);

	auto& ecs = Ecs::GetInstance();
	ecs.AddSingletonComponent(GPUSceneData{});
	auto sceneData = ecs.GetSingletonComponent<GPUSceneData>();
	sceneData->ambientColor = glm::vec4(.1f);
	sceneData->sunlightColor = glm::vec4(1.f);
	sceneData->sunlightDirection = glm::vec4(0,1,0.5,1.f);
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, std::vector<RenderObject>& objects)
{
	// Setup
	VkCommandBufferBeginInfo cmdBeginInfo = VkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkViewport viewport {
    	.x = 0.0f,
    	.y = 0.0f,
    	.width = static_cast<float>(m_swapchain.GetExtent().width),
    	.height = static_cast<float>(m_swapchain.GetExtent().height),
    	.minDepth = 0.0f,
    	.maxDepth = 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor {
    	.offset = {0, 0},
    	.extent = m_swapchain.GetExtent()
    };
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// Record draw commands
	VkUtil::transition_image(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	VkUtil::transition_image(cmd, m_swapchain.GetDepthImage().GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkImageSubresourceRange clearRange = VkInit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdClearColorImage(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValues[0].color, 1, &clearRange);
	drawObjects(cmd, objects);

	VkUtil::transition_image(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	VkUtil::transition_image(cmd, m_swapchain.GetImage(imageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkExtent2D drawExtent {
		.width = static_cast<uint32_t>(std::min(m_swapchain.GetExtent().width, m_swapchain.GetDrawImage().GetExtent().width) * m_swapchain.GetRenderScale()),
		.height = static_cast<uint32_t>(std::min(m_swapchain.GetExtent().height, m_swapchain.GetDrawImage().GetExtent().height) * m_swapchain.GetRenderScale())
	};
	VkUtil::copy_image_to_image(cmd, m_swapchain.GetDrawImage().GetImage(), m_swapchain.GetImage(imageIndex), drawExtent, m_swapchain.GetExtent());

	// Imgui
	VkUtil::copy_image_to_image(cmd, m_swapchain.GetDrawImage().GetImage(), m_swapchain.GetImage(imageIndex), drawExtent, m_swapchain.GetExtent());
	VkUtil::transition_image(cmd, m_swapchain.GetImage(imageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	drawImgui(cmd, m_swapchain.GetImageView(imageIndex));

	VkUtil::transition_image(cmd, m_swapchain.GetImage(imageIndex), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	VK_CHECK(vkEndCommandBuffer(cmd));
}

VkCommandBuffer Renderer::beginSingleTimeCommands(VkCommandPool& commandPool) const
{
	VkCommandBufferAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_ctx->GetDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandPool& commandPool, VkCommandBuffer& commandBuffer) const
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};

	vkQueueSubmit(m_ctx->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_ctx->GetGraphicsQueue());

	vkFreeCommandBuffers(m_ctx->GetDevice(), commandPool, 1, &commandBuffer);
}

void Renderer::drawImgui(VkCommandBuffer cmd, VkImageView targetImageView) const
{
	VkRenderingAttachmentInfo colorAttachment = VkInit::color_attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = VkInit::rendering_info(m_swapchain.GetExtent(), &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void Renderer::drawObjects(VkCommandBuffer cmd, std::vector<RenderObject>& objects)
{
	VkRenderingAttachmentInfo colorAttachment = VkInit::color_attachment_info(m_swapchain.GetDrawImage().GetView(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingAttachmentInfo depthAttachment = VkInit::depth_attachment_info(m_swapchain.GetDepthImage().GetView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkExtent2D drawExtent = {m_swapchain.GetDrawImage().GetExtent().width, m_swapchain.GetDrawImage().GetExtent().height};
	VkRenderingInfo renderInfo = VkInit::rendering_info(drawExtent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(cmd, &renderInfo);

	Buffer gpuSceneDataBuffer(m_allocator, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	GPUSceneData* sceneUniformData = static_cast<GPUSceneData*>(gpuSceneDataBuffer.allocation->GetMappedData());
	*sceneUniformData = *Ecs::GetInstance().GetSingletonComponent<GPUSceneData>();
	VkDescriptorSet globalDescriptor = getCurrentFrame().frameDescriptors.Allocate(m_ctx->GetDevice(), m_gpuSceneDataDescriptorLayout);

	{
		DescriptorWriter writer;
		writer.WriteBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.UpdateSet(m_ctx->GetDevice(), globalDescriptor);
	}

	//TODO: Fix this so that no shared_ptr necessary (probably upgrade the deletion queue)
	getCurrentFrame().deletionQueue.PushFunction([buffer = std::make_shared<Buffer>(std::move(gpuSceneDataBuffer))]() mutable {
		buffer->Cleanup();
	});

	for (auto& [indexCount, firstIndex, indexBuffer, material, transform, vertexBufferAddress] : objects)
	{
		material = &m_defaultData; // TODO: This is not the right way to do this
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline->pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr );
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline->layout, 1, 1, &material->materialSet, 0, nullptr );

		vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		GPUDrawPushConstants pushConstants {
			.worldMatrix = transform,
			.vertexBuffer = vertexBufferAddress
		};
		vkCmdPushConstants(cmd, material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);

		vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, 0, 0);
	}

	vkCmdEndRendering(cmd);
}

FrameData& Renderer::getCurrentFrame() { return m_frames[m_currentFrame % FRAME_OVERLAP]; }

VkRenderingAttachmentInfo Renderer::attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout)
{
	VkRenderingAttachmentInfo colorAttachment {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext = nullptr,
		.imageView = view,
		.imageLayout = layout,
		.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	};

	if (clear)
		colorAttachment.clearValue = *clear;

	return colorAttachment;
}

void Renderer::DrawFrame(std::vector<RenderObject>& objects)
{
    VK_CHECK(vkWaitForFences(m_ctx->GetDevice(), 1, &getCurrentFrame().renderFence, true, UINT64_MAX));

	getCurrentFrame().deletionQueue.Flush();
	getCurrentFrame().frameDescriptors.ClearPools(m_ctx->GetDevice());
	VK_CHECK(vkResetFences(m_ctx->GetDevice(), 1, &getCurrentFrame().renderFence));

	getCurrentFrame().deletionQueue.Flush();

	if (m_swapchain.IsResized())
	{
		m_swapchain.RecreateSwapchain();
		m_swapchain.SetResized(false);
	}

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_ctx->GetDevice(), m_swapchain.GetSwapchain(), UINT64_MAX, getCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_swapchain.SetResized(true);
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("failed to acquire swap chain image!");

	VK_CHECK(vkResetFences(m_ctx->GetDevice(), 1, &getCurrentFrame().renderFence));
	VK_CHECK(vkResetCommandBuffer(getCurrentFrame().commandBuffer, 0));

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();

	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
	recordCommandBuffer(cmd, imageIndex, objects);

	VkCommandBufferSubmitInfo cmdInfo = VkInit::command_buffer_submit_info(cmd);

	VkSemaphoreSubmitInfo waitInfo = VkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,getCurrentFrame().swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = VkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

	VkSubmitInfo2 submit = VkInit::submit_info(&cmdInfo,&signalInfo,&waitInfo);

	VK_CHECK(vkQueueSubmit2(m_ctx->GetGraphicsQueue(), 1, &submit, getCurrentFrame().renderFence));

	VkPresentInfoKHR presentInfo {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &getCurrentFrame().renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &m_swapchain.GetSwapchain(),
		.pImageIndices = &imageIndex
	};

    result = vkQueuePresentKHR(m_ctx->GetPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_swapchain.IsResized())
    	m_swapchain.SetResized(true);
    else if (result != VK_SUCCESS)
        throw std::runtime_error("failed to present swap chain image!");

    m_currentFrame = (m_currentFrame + 1) % FRAME_OVERLAP;
}

std::shared_ptr<GPUMeshBuffers> Renderer::UploadMesh(const std::span<uint32_t> indices, const std::span<Vertex> vertices) const
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	Buffer vertexBuffer(m_allocator, vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Buffer indexBuffer(m_allocator, indexBufferSize,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	VkBufferDeviceAddressInfo deviceAddressInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = vertexBuffer.buffer
	};

	std::shared_ptr<GPUMeshBuffers> newSurface = std::make_shared<GPUMeshBuffers> (
		std::move(vertexBuffer),
		std::move(indexBuffer),
		vkGetBufferDeviceAddress(m_ctx->GetDevice(), &deviceAddressInfo)
		);

	Buffer staging(m_allocator, vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	void* data = staging.allocation->GetMappedData();
	if (!data)
		throw std::runtime_error("Failed to allocate staging buffer");

	memcpy(data, vertices.data(), vertexBufferSize);
	memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);

	// TODO: put on a background thread
	m_ctx->ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = vertexBufferSize
		};

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface->vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy {
			.srcOffset = vertexBufferSize,
			.dstOffset = 0,
			.size = indexBufferSize
		};

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface->indexBuffer.buffer, 1, &indexCopy);
	});

	return newSurface;
}

std::optional<std::vector<std::shared_ptr<Mesh>>> Renderer::LoadGltfMeshes(const std::filesystem::path& filePath) const
{
	if (!std::filesystem::exists(filePath))
	{
		std::print("Failed to load mesh, path doesn't exist: {}", std::filesystem::absolute(filePath).string());
		return {};
	}

	auto data = fastgltf::MappedGltfFile::FromPath(filePath);

	constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

	fastgltf::Asset gltf;
	fastgltf::Parser parser {};
	auto load = parser.loadGltfBinary(data.get(), filePath.parent_path(), gltfOptions);
	if (load) {
		gltf = std::move(load.get());
	} else {
		std::print("Failed to load glTF: {} \n", fastgltf::to_underlying(load.error()));
		return {};
	}

	std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        Mesh newMesh;
        newMesh.name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            GeoSurface newSurface{};
            newSurface.startIndex = static_cast<uint32_t>(indices.size());
            newSurface.count = static_cast<uint32_t>(gltf.accessors[p.indicesAccessor.value()].count);

            size_t initial_vtx = vertices.size();

            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexAccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor,
                    [&](std::uint32_t idx) {
                        indices.push_back(idx + initial_vtx);
                    });
            }

            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex newVertex;
                        newVertex.position = v;
                        newVertex.normal = { 1, 0, 0 };
                        newVertex.color = glm::vec4 { 1.f };
                    	newVertex.uv_x = 0;
                        newVertex.uv_y = 0;
                        vertices[initial_vtx + index] = newVertex;
                    });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[normals->accessorIndex],
                    [&](glm::vec3 v, size_t index) {
                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[uv->accessorIndex],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initial_vtx + index].uv_x = v.x;
                        vertices[initial_vtx + index].uv_y = v.y;
                    });
            }


        	// load vertex colors
        	auto colors = p.findAttribute("COLOR_0");
        	if (colors != p.attributes.end()) {

        		fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[colors->accessorIndex],
					[&](glm::vec4 v, size_t index) {
						vertices[initial_vtx + index].color = v;
					});
        	}

            newMesh.surfaces.push_back(newSurface);
        }

        if (constexpr bool OverrideColors = true) {
            for (Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }

    	newMesh.meshBuffers = UploadMesh(indices, vertices);
        meshes.emplace_back(std::make_shared<Mesh>(std::move(newMesh)));
    }

    return meshes;
}
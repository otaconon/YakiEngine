#include "Vulkan/Renderer.h"

#include <imgui.h>
#include <stdexcept>
#include <SDL3/SDL_vulkan.h>
#include <ranges>

#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL_mouse.h>

#include "Vulkan/ImGuiStyles.h"
#include "Vulkan/PipelineBuilder.h"
#include "Vulkan/VkInit.h"
#include "Vulkan/Descriptors/DescriptorLayoutBuilder.h"
#include "Components/RenderComponents.h"
#include "Vulkan/DefaultData.h"
#include "Vulkan/Descriptors/DescriptorWriter.h"

Renderer::Renderer(SDL_Window* window, VulkanContext* ctx)
    : m_window{window},
    m_ctx{ctx},
    m_swapchain{m_ctx, window},
    m_currentFrame{0},
	m_materialConstantsBuffer{m_ctx->GetAllocator(), sizeof(MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU}
{
	initCommands();
	initSyncObjects();
	initImgui();
	initDescriptorAllocator();
	initDescriptors();
	initPicking();
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

void Renderer::BeginRendering()
{
	// Synchronize
	VK_CHECK(vkWaitForFences(m_ctx->GetDevice(), 1, &getCurrentFrame().renderFence, true, UINT64_MAX));

	getCurrentFrame().deletionQueue.Flush();
	getCurrentFrame().frameDescriptors.ClearPools(m_ctx->GetDevice());
	VK_CHECK(vkResetFences(m_ctx->GetDevice(), 1, &getCurrentFrame().renderFence));
	VK_CHECK(vkResetCommandBuffer(getCurrentFrame().commandBuffer, 0));

	// Setup swapchain
	if (m_swapchain.IsResized())
	{
		m_swapchain.RecreateSwapchain();
		m_swapchain.SetResized(false);
	}

	VkResult result = vkAcquireNextImageKHR(m_ctx->GetDevice(), m_swapchain.GetSwapchain(), UINT64_MAX, getCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &m_currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_swapchain.SetResized(true);
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("failed to acquire swap chain image!");

	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = VkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// Setup image layout
	VkUtil::transition_image(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	VkUtil::transition_image(cmd, m_swapchain.GetDepthImage().GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	VkUtil::transition_image(cmd, m_pickingResources.texture->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// Clear image
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	VkImageSubresourceRange clearRange = VkInit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdClearColorImage(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValues[0].color, 1, &clearRange);
	vkCmdClearColorImage(cmd, m_pickingResources.texture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValues[0].color, 1, &clearRange);

	// Reset rendering stats
	m_stats = RenderingStats{0, 0, 0};
}

void Renderer::Begin3DRendering()
{
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
	std::array<VkRenderingAttachmentInfo, 2> colorAttachments {
		VkInit::color_attachment_info(m_swapchain.GetDrawImage().GetView(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		VkInit::color_attachment_info(m_pickingResources.texture->GetView(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	};
	VkRenderingAttachmentInfo depthAttachment = VkInit::depth_attachment_info(m_swapchain.GetDepthImage().GetView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkExtent2D drawExtent = {m_swapchain.GetDrawImage().GetExtent().width, m_swapchain.GetDrawImage().GetExtent().height};
	VkRenderingInfo renderInfo = VkInit::rendering_info(drawExtent, colorAttachments, &depthAttachment);
	vkCmdBeginRendering(cmd, &renderInfo);

	// Scene data
	Buffer gpuSceneDataBuffer(m_ctx->GetAllocator(), sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	gpuSceneDataBuffer.MapMemoryFromValue(m_gpuSceneData);

	// Light data
	Buffer lightBuffer(m_ctx->GetAllocator(), sizeof(GPULightData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	lightBuffer.MapMemoryFromValue(m_gpuLightData);

	m_globalDescriptor = getCurrentFrame().frameDescriptors.Allocate(m_ctx->GetDevice(), m_gpuSceneDataDescriptorLayout);
	{
		DescriptorWriter writer;
		writer.WriteBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.WriteBuffer(1, lightBuffer.buffer, sizeof(GPULightData), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		writer.UpdateSet(m_ctx->GetDevice(), m_globalDescriptor);
	}

	getCurrentFrame().deletionQueue.PushBuffer(std::move(gpuSceneDataBuffer));
	getCurrentFrame().deletionQueue.PushBuffer(std::move(lightBuffer));
}

void Renderer::RenderObjects(std::span<RenderObject> objects, std::span<size_t> order)
{
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

	MaterialPipeline* lastPipeline{nullptr};
	MaterialInstance* lastMaterial{nullptr};
	VkBuffer lastIndexBuffer{VK_NULL_HANDLE};
	for (auto& idx : order)
	{
		auto& [objectId, indexCount, firstIndex, indexBuffer, material, bounds, transform, vertexBufferAddress] = objects[idx];
		if (material != lastMaterial)
		{
			lastMaterial = material;
			if (material->pipeline != lastPipeline)
			{
				lastPipeline = material->pipeline;
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline->pipeline);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline->layout, 0, 1, &m_globalDescriptor, 0, nullptr );

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
			}
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline->layout, 1, 1, &material->materialSet, 0, nullptr );
		}

		if (lastIndexBuffer != indexBuffer)
		{
			lastIndexBuffer = indexBuffer;
			vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}

		GPUDrawPushConstants pushConstants {
			.worldMatrix = transform,
			.vertexBuffer = vertexBufferAddress,
			.objectId = objectId
		};
		vkCmdPushConstants(cmd, material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);

		vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, 0, 0);
		m_stats.drawcallCount++;
		m_stats.triangleCount += indexCount / 3;
	}
}

void Renderer::End3DRendering()
{
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
	vkCmdEndRendering(cmd);

	// Handle draw image
	VkUtil::transition_image(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	VkUtil::transition_image(cmd, m_swapchain.GetImage(m_currentImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Handle object picking
	VkUtil::transition_image(cmd, m_pickingResources.texture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	VkBufferImageCopy region{};
	float mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	region.imageOffset = {static_cast<int>(mouseX), static_cast<int>(mouseY), 0};
	region.imageExtent = {1, 1, 1};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	vkCmdCopyImageToBuffer(cmd, m_pickingResources.texture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_pickingResources.stagingBuffer->buffer, 1, &region);
}

void Renderer::RenderImGui()
{
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
	VkExtent2D drawExtent = {
		.width = static_cast<uint32_t>(std::min(m_swapchain.GetExtent().width, m_swapchain.GetDrawImage().GetExtent().width) * m_swapchain.GetRenderScale()),
		.height = static_cast<uint32_t>(std::min(m_swapchain.GetExtent().height, m_swapchain.GetDrawImage().GetExtent().height) * m_swapchain.GetRenderScale())
	};

	VkUtil::copy_image_to_image(cmd, m_swapchain.GetDrawImage().GetImage(), m_swapchain.GetImage(m_currentImageIndex), drawExtent, m_swapchain.GetExtent());
	VkUtil::transition_image(cmd, m_swapchain.GetImage(m_currentImageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	std::array colorAttachments { VkInit::color_attachment_info(m_swapchain.GetImageView(m_currentImageIndex), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) };
	VkRenderingInfo renderInfo = VkInit::rendering_info(m_swapchain.GetExtent(), colorAttachments, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	vkCmdEndRendering(cmd);
}

void Renderer::EndRendering()
{
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

	VkUtil::transition_image(cmd, m_swapchain.GetImage(m_currentImageIndex), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdInfo = VkInit::command_buffer_submit_info(getCurrentFrame().commandBuffer);
	VkSemaphoreSubmitInfo waitInfo = VkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = VkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);
	VkSubmitInfo2 submit = VkInit::submit_info(&cmdInfo, &signalInfo, &waitInfo);

	VK_CHECK(vkQueueSubmit2(m_ctx->GetGraphicsQueue(), 1, &submit, getCurrentFrame().renderFence));

	VkPresentInfoKHR presentInfo {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &getCurrentFrame().renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &m_swapchain.GetSwapchain(),
		.pImageIndices = &m_currentImageIndex
	};

	VkResult result = vkQueuePresentKHR(m_ctx->GetPresentQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_swapchain.IsResized())
		m_swapchain.SetResized(true);
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swap chain image!");

	m_pickingResources.stagingBuffer->MapMemoryToValue(m_pickingResources.entityId);
	m_currentFrame = (m_currentFrame + 1) % FRAME_OVERLAP;
}

Swapchain& Renderer::GetSwapchain()
{
	return m_swapchain;
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
		builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		m_gpuSceneDataDescriptorLayout = builder.Build(m_ctx->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		m_deletionQueue.PushFunction([&] {
			vkDestroyDescriptorSetLayout(m_ctx->GetDevice(), m_gpuSceneDataDescriptorLayout, nullptr);
		});
	}
}

void Renderer::initPicking()
{
	m_pickingResources.texture = std::make_shared<Texture>(m_ctx, m_ctx->GetAllocator(), VkExtent3D{m_swapchain.GetExtent().width, m_swapchain.GetExtent().height, 1}, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, false);
	m_pickingResources.stagingBuffer = std::make_shared<Buffer>(m_ctx->GetAllocator(), sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
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

FrameData& Renderer::getCurrentFrame() { return m_frames[m_currentFrame % FRAME_OVERLAP]; }

void Renderer::WaitIdle()
{
	vkDeviceWaitIdle(m_ctx->GetDevice());
}

VkBuffer Renderer::GetMaterialConstantsBuffer()
{
	return m_materialConstantsBuffer.buffer;
}

VkDescriptorSetLayout Renderer::GetSceneDataDescriptorLayout()
{
	return m_gpuSceneDataDescriptorLayout;
}

uint32_t Renderer::GetHoveredEntityId()
{
	return m_pickingResources.entityId;
}

RenderingStats Renderer::GetRenderingStats()
{
	return m_stats;
}

GPUSceneData& Renderer::GetGpuSceneData() {
    return m_gpuSceneData;
}

GPULightData& Renderer::GetGpuLightData() {
    return m_gpuLightData;
}
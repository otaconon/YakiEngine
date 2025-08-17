#include "Renderer.h"

#include <imgui.h>
#include <stdexcept>
#include <SDL3/SDL_vulkan.h>
#include <ranges>

#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "ImGuiStyles.h"
#include "VkInit.h"
#include "../Ecs.h"
#include "Descriptors/DescriptorLayoutBuilder.h"
#include "../Components/Components.h"
#include "../DefaultData.h"
#include "Descriptors/DescriptorWriter.h"

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

	VkResult result = vkAcquireNextImageKHR(m_ctx->GetDevice(), m_swapchain.GetSwapchain(), UINT64_MAX, getCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &m_currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_swapchain.SetResized(true);
		return;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("failed to acquire swap chain image!");

	VK_CHECK(vkResetFences(m_ctx->GetDevice(), 1, &getCurrentFrame().renderFence));
	VK_CHECK(vkResetCommandBuffer(getCurrentFrame().commandBuffer, 0));

	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = VkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

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

	// Setup image layout
	VkUtil::transition_image(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	VkUtil::transition_image(cmd, m_swapchain.GetDepthImage().GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	VkImageSubresourceRange clearRange = VkInit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdClearColorImage(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValues[0].color, 1, &clearRange);
}

void Renderer::RenderObjects(std::vector<RenderObject>& objects)
{
	VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
	VkRenderingAttachmentInfo colorAttachment = VkInit::color_attachment_info(m_swapchain.GetDrawImage().GetView(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingAttachmentInfo depthAttachment = VkInit::depth_attachment_info(m_swapchain.GetDepthImage().GetView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkExtent2D drawExtent = {m_swapchain.GetDrawImage().GetExtent().width, m_swapchain.GetDrawImage().GetExtent().height};
	VkRenderingInfo renderInfo = VkInit::rendering_info(drawExtent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(cmd, &renderInfo);

	// Scene data
	Buffer gpuSceneDataBuffer(m_ctx->GetAllocator(), sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	GPUSceneData* sceneUniformData;
	vmaMapMemory(gpuSceneDataBuffer.allocator, gpuSceneDataBuffer.allocation, reinterpret_cast<void**>(&sceneUniformData));
	*sceneUniformData = *Ecs::GetInstance().GetSingletonComponent<GPUSceneData>();
	vmaUnmapMemory(gpuSceneDataBuffer.allocator, gpuSceneDataBuffer.allocation);

	// Light buffer
	Buffer lightBuffer(m_ctx->GetAllocator(), sizeof(GPULightData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto lightData = *Ecs::GetInstance().GetSingletonComponent<GPULightData>();
	GPULightData* mappedLightData;
	vmaMapMemory(lightBuffer.allocator, lightBuffer.allocation, reinterpret_cast<void**>(&mappedLightData));
	*mappedLightData = lightData;
	vmaUnmapMemory(lightBuffer.allocator, lightBuffer.allocation);

	VkDescriptorSet globalDescriptor = getCurrentFrame().frameDescriptors.Allocate(m_ctx->GetDevice(), m_gpuSceneDataDescriptorLayout);
	{
		DescriptorWriter writer;
		writer.WriteBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.WriteBuffer(1, lightBuffer.buffer, sizeof(GPULightData), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		writer.UpdateSet(m_ctx->GetDevice(), globalDescriptor);
	}
	m_deletionQueue.PushBuffer(std::move(gpuSceneDataBuffer));
	m_deletionQueue.PushBuffer(std::move(lightBuffer));

	for (auto& [indexCount, firstIndex, indexBuffer, material, transform, vertexBufferAddress] : objects)
	{
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

	VkUtil::transition_image(cmd, m_swapchain.GetDrawImage().GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	VkUtil::transition_image(cmd, m_swapchain.GetImage(m_currentImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
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

	VkRenderingAttachmentInfo colorAttachment = VkInit::color_attachment_info(m_swapchain.GetImageView(m_currentImageIndex), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = VkInit::rendering_info(m_swapchain.GetExtent(), &colorAttachment, nullptr);

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
		.pImageIndices = &m_currentImageIndex
	};

	VkResult result = vkQueuePresentKHR(m_ctx->GetPresentQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_swapchain.IsResized())
		m_swapchain.SetResized(true);
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swap chain image!");

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

void Renderer::WriteMaterialConstants(MaterialConstants& materialConstants)
{
	vmaMapMemory(m_materialConstantsBuffer.allocator, m_materialConstantsBuffer.allocation, reinterpret_cast<void**>(&materialConstants));
	materialConstants.colorFactors = glm::vec4{1,1,1,1};
	materialConstants.metalRoughtFactors = glm::vec4{1,0.5,0,0};
	vmaUnmapMemory(m_materialConstantsBuffer.allocator, m_materialConstantsBuffer.allocation);
}
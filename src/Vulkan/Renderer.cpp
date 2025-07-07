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

#include "ImGuiStyles.h"
#include "VkInit.h"
#include "../Ecs.h"

Renderer::Renderer(SDL_Window* window, const std::shared_ptr<VulkanContext>& ctx)
    : m_window(window),
    m_ctx(ctx),
    m_swapchain(ctx, window),
	m_graphicsPipeline(ctx, m_swapchain),
    m_currentFrame(0)
{
	initCommands();
	initSyncObjects();
	initImgui();

	VmaAllocatorCreateInfo allocatorInfo {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = m_ctx->GetPhysicalDevice(),
		.device = m_ctx->GetDevice(),
		.instance = m_ctx->GetInstance(),
	};
	vmaCreateAllocator(&allocatorInfo, &m_allocator);

	m_deletionQueue.PushFunction([&]() {
		vmaDestroyAllocator(m_allocator);
	});

	VkShaderModule fragmentShader;
	if (!VkUtil::load_shader_module("../shaders/fragment/first_triangle.frag.spv", m_ctx->GetDevice(), &fragmentShader))
		std::println("Error when building the triangle fragment shader module");
	else
		std::println("Triangle fragment shader succesfully loaded");

	VkShaderModule vertexShader;
	if (!VkUtil::load_shader_module("../shaders/vertex/first_triangle.vert.spv", m_ctx->GetDevice(), &vertexShader))
		std::println("Error when building the triangle vertex shader module");
	else
		std::println("Triangle vertex shader succesfully loaded");

	VkPushConstantRange bufferRange {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(GPUDrawPushConstants),
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = VkInit::pipeline_layout_create_info();
	pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	VK_CHECK(vkCreatePipelineLayout(m_ctx->GetDevice(), &pipelineLayoutInfo, nullptr, &m_graphicsPipelineLayout));

	m_graphicsPipeline.SetLayout(m_graphicsPipelineLayout);
	m_graphicsPipeline.SetShaders(vertexShader, fragmentShader);
	m_graphicsPipeline.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	m_graphicsPipeline.SetPolygonMode(VK_POLYGON_MODE_FILL);
	m_graphicsPipeline.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	m_graphicsPipeline.SetMultisamplingNone();
	m_graphicsPipeline.DisableBlending();
	m_graphicsPipeline.DisableDepthTest();
	m_graphicsPipeline.SetColorAttachmentFormat(m_swapchain.GetDrawImage().imageFormat);
	m_graphicsPipeline.SetDepthFormat(VK_FORMAT_UNDEFINED);

	m_graphicsPipeline.CreateGraphicsPipeline();

	vkDestroyShaderModule(m_ctx->GetDevice(), fragmentShader, nullptr);
	vkDestroyShaderModule(m_ctx->GetDevice(), vertexShader, nullptr);

	m_deletionQueue.PushFunction([&]() {
		vkDestroyPipelineLayout(m_ctx->GetDevice(), m_graphicsPipelineLayout, nullptr);
	});
}

Renderer::~Renderer()
{
    const VkDevice device = m_ctx->GetDevice();
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
	QueueFamilyIndices queueFamilyIndices = VkUtil::find_queue_families(m_ctx->GetPhysicalDevice(), m_ctx->GetSurface());
	VkCommandPoolCreateInfo commandPoolInfo = VkInit::command_pool_create_info(queueFamilyIndices.graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(m_ctx->GetDevice(), &commandPoolInfo, nullptr, &m_frames[i].commandPool));
		VkCommandBufferAllocateInfo cmdAllocInfo = VkInit::command_buffer_allocate_info(m_frames[i].commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(m_ctx->GetDevice(), &cmdAllocInfo, &m_frames[i].commandBuffer));
	}

	VK_CHECK(vkCreateCommandPool(m_ctx->GetDevice(), &commandPoolInfo, nullptr, &m_immCommandPool));
	VkCommandBufferAllocateInfo cmdAllocInfo = VkInit::command_buffer_allocate_info(m_immCommandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(m_ctx->GetDevice(), &cmdAllocInfo, &m_immCommandBuffer));

	m_deletionQueue.PushFunction([=]() { vkDestroyCommandPool(m_ctx->GetDevice(), m_immCommandPool, nullptr); });
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
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = (uint32_t)std::size(pool_sizes),
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

	m_deletionQueue.PushFunction([=]() {
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

	VK_CHECK(vkCreateFence(m_ctx->GetDevice(), &fenceCreateInfo, nullptr, &m_immFence));
	m_deletionQueue.PushFunction([=]() { vkDestroyFence(m_ctx->GetDevice(), m_immFence, nullptr); });
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, std::vector<Drawable>& drawables)
{
	// Setup
	VkCommandBufferBeginInfo cmdBeginInfo = VkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    std::array<VkClearValue, 2> clearValues;
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
	VkUtil::transition_image(cmd, m_swapchain.GetDrawImage().image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	VkImageSubresourceRange clearRange = VkInit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
	vkCmdClearColorImage(cmd, m_swapchain.GetDrawImage().image, VK_IMAGE_LAYOUT_GENERAL, &clearValues[0].color, 1, &clearRange);
	drawObjects(cmd, drawables);

	VkUtil::transition_image(cmd, m_swapchain.GetDrawImage().image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	VkUtil::transition_image(cmd, m_swapchain.GetImage(imageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	VkExtent2D drawExtent = {m_swapchain.GetDrawImage().imageExtent.width, m_swapchain.GetDrawImage().imageExtent.height};
	VkUtil::copy_image_to_image(cmd, m_swapchain.GetDrawImage().image, m_swapchain.GetImage(imageIndex), drawExtent, m_swapchain.GetExtent());

	// Imgui
	VkUtil::copy_image_to_image(cmd, m_swapchain.GetDrawImage().image, m_swapchain.GetImage(imageIndex), drawExtent, m_swapchain.GetExtent());
	VkUtil::transition_image(cmd, m_swapchain.GetImage(imageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	drawImgui(cmd, m_swapchain.GetImageView(imageIndex));

	VkUtil::transition_image(cmd, m_swapchain.GetImage(imageIndex), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	VK_CHECK(vkEndCommandBuffer(cmd));
}

VkCommandBuffer Renderer::beginSingleTimeCommands(VkCommandPool& commandPool)
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

void Renderer::endSingleTimeCommands(VkCommandPool& commandPool, VkCommandBuffer& commandBuffer)
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

void Renderer::drawImgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo colorAttachment = VkInit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = VkInit::rendering_info(m_swapchain.GetExtent(), &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void Renderer::drawObjects(VkCommandBuffer cmd, std::vector<Drawable>& drawables)
{
	VkRenderingAttachmentInfo colorAttachment = VkInit::attachment_info(m_swapchain.GetDrawImage().imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkExtent2D drawExtent = {m_swapchain.GetDrawImage().imageExtent.width, m_swapchain.GetDrawImage().imageExtent.height};
	VkRenderingInfo renderInfo = VkInit::rendering_info(drawExtent, &colorAttachment, nullptr);
	vkCmdBeginRendering(cmd, &renderInfo);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.GetGraphicsPipeline());

	VkViewport viewport {
		.x = 0,
		.y = 0,
		.width = static_cast<float>(drawExtent.width),
		.height = static_cast<float>(drawExtent.width),
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor {
		.offset = {0, 0},
		.extent = {m_swapchain.GetDrawImage().imageExtent.width, m_swapchain.GetDrawImage().imageExtent.height}
	};

	vkCmdSetScissor(cmd, 0, 1, &scissor);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.GetGraphicsPipeline());

	GPUDrawPushConstants push_constants {
		.model = drawables[0].ubo.model,
		.view = drawables[0].ubo.view,
		.proj = drawables[0].ubo.proj,
		.vertexBuffer = drawables[0].mesh->vertexBufferAddress,
	};

	VkBuffer vertexBuffers[] = {drawables[0].mesh->vertexBuffer.buffer};
	VkDeviceSize offsets[] = { 0 };
	vkCmdPushConstants(cmd, m_graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cmd, drawables[0].mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmd, static_cast<uint32_t>(drawables[0].indices.size()), 1, 0, 0, 0);

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

void Renderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(m_ctx->GetDevice(), 1, &m_immFence));
	VK_CHECK(vkResetCommandBuffer(m_immCommandBuffer, 0));

	VkCommandBuffer cmd = m_immCommandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = VkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	function(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = VkInit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = VkInit::submit_info(&cmdinfo, nullptr, nullptr);

	VK_CHECK(vkQueueSubmit2(m_ctx->GetGraphicsQueue(), 1, &submit, m_immFence));
	VK_CHECK(vkWaitForFences(m_ctx->GetDevice(), 1, &m_immFence, true, 9999999999));
}

void Renderer::DrawFrame(std::vector<Drawable>& drawables)
{
    VK_CHECK(vkWaitForFences(m_ctx->GetDevice(), 1, &getCurrentFrame().renderFence, true, UINT64_MAX));
	VK_CHECK(vkResetFences(m_ctx->GetDevice(), 1, &getCurrentFrame().renderFence));

	getCurrentFrame().deletionQueue.Flush();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_ctx->GetDevice(), m_swapchain.GetSwapchain(), UINT64_MAX, getCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_swapchain.RecreateSwapchain();
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
	recordCommandBuffer(cmd, imageIndex, drawables);

	VkCommandBufferSubmitInfo cmdinfo = VkInit::command_buffer_submit_info(cmd);

	VkSemaphoreSubmitInfo waitInfo = VkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,getCurrentFrame().swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = VkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

	VkSubmitInfo2 submit = VkInit::submit_info(&cmdinfo,&signalInfo,&waitInfo);

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
    {
        m_swapchain.SetResized(false);
        m_swapchain.RecreateSwapchain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % FRAME_OVERLAP;
}

std::shared_ptr<GPUMeshBuffers> Renderer::UploadMesh(const std::span<uint32_t> indices, const std::span<Vertex> vertices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	Buffer vertexBuffer(m_allocator, vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Buffer indexBuffer(m_allocator, indexBufferSize,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	VkBufferDeviceAddressInfo deviceAdressInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = vertexBuffer.buffer
	};

	std::shared_ptr<GPUMeshBuffers> newSurface = std::make_shared<GPUMeshBuffers> (
		std::move(vertexBuffer),
		std::move(indexBuffer),
		vkGetBufferDeviceAddress(m_ctx->GetDevice(), &deviceAdressInfo)
		);

	Buffer staging(m_allocator, vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data = staging.allocation->GetMappedData();
	memcpy(data, vertices.data(), vertexBufferSize);
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	// TODO: put on a background thread
	immediateSubmit([&](VkCommandBuffer cmd) {
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

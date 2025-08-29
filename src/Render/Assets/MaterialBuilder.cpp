#include "Vulkan/PipelineBuilder.h"
#include "Assets/MaterialBuilder.h"

MaterialBuilder::MaterialBuilder(VulkanContext* ctx)
	: m_ctx(ctx)
{}

MaterialBuilder::~MaterialBuilder()
{
	vkDestroyPipeline(m_ctx->GetDevice(), m_opaquePipeline.pipeline, nullptr);
	vkDestroyPipeline(m_ctx->GetDevice(), m_transparentPipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(m_ctx->GetDevice(), m_opaquePipeline.layout, nullptr);
	vkDestroyDescriptorSetLayout(m_ctx->GetDevice(), m_materialLayout, nullptr);
}

void MaterialBuilder::BuildPipelines(Swapchain& swapchain, VkDescriptorSetLayout gpuSceneDataDescriptorLayout)
{
    VkShaderModule meshFragShader;
	if (!VkUtil::load_shader_module("../shaders/fragment/materials.frag.spv", m_ctx->GetDevice(), &meshFragShader))
		std::println("Error when building the triangle fragment shader module");

	VkShaderModule meshVertexShader;
	if (!VkUtil::load_shader_module("../shaders/vertex/materials.vert.spv", m_ctx->GetDevice(), &meshVertexShader))
		std::println("Error when building the triangle vertex shader module");

	VkPushConstantRange matrixRange {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(GPUDrawPushConstants)
	};

    DescriptorLayoutBuilder layoutBuilder{};
    layoutBuilder.AddBinding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    m_materialLayout = layoutBuilder.Build(m_ctx->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layouts[] = { gpuSceneDataDescriptorLayout, m_materialLayout };

	VkPipelineLayoutCreateInfo mesh_layout_info = VkInit::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(m_ctx->GetDevice(), &mesh_layout_info, nullptr, &newLayout));
	m_opaquePipeline.layout = newLayout;
	m_transparentPipeline.layout = newLayout;

	PipelineBuilder pipelineBuilder(m_ctx);
	pipelineBuilder.SetShaders(meshVertexShader, meshFragShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultisamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthTest(true);
	pipelineBuilder.SetColorAttachmentFormat(swapchain.GetDrawImage().GetFormat());
	pipelineBuilder.SetDepthFormat(swapchain.GetDepthImage().GetFormat());
	pipelineBuilder.SetLayout(newLayout);

	std::array<VkPipelineColorBlendAttachmentState, 2> blendAttachments;
	blendAttachments[0] = {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	blendAttachments[1] = {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
	};

	std::array<VkFormat, 2> colorFormats {
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT
	};

	m_opaquePipeline.pipeline = pipelineBuilder.CreateMRTPipeline(blendAttachments, colorFormats);

	pipelineBuilder.EnableBlendingAdditive();
	pipelineBuilder.EnableDepthTest(false);
	m_transparentPipeline.pipeline = pipelineBuilder.CreateMRTPipeline(blendAttachments, colorFormats);

	vkDestroyShaderModule(m_ctx->GetDevice(), meshFragShader, nullptr);
	vkDestroyShaderModule(m_ctx->GetDevice(), meshVertexShader, nullptr);
}

MaterialInstance MaterialBuilder::WriteMaterial(MaterialPass pass, const MaterialResources& resources, DescriptorAllocator& descriptorAllocator)
{
	MaterialInstance matData{};
	matData.passType = pass;
	if (pass == MaterialPass::Transparent) {
		matData.pipeline = &m_transparentPipeline;
	}
	else {
		matData.pipeline = &m_opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.Allocate(m_ctx->GetDevice(), m_materialLayout);

	m_writer.Clear();
	m_writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	m_writer.WriteImage(1, resources.colorImage->GetView(), resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	m_writer.WriteImage(2, resources.metalRoughImage->GetView(), resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	m_writer.UpdateSet(m_ctx->GetDevice(), matData.materialSet);

	return matData;
}

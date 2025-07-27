#include "MetallicRoughnessMaterial.h"

#include "GraphicsPipeline.h"
#include "Swapchain.h"
#include "Descriptors/DescriptorLayoutBuilder.h"

MetallicRoughnessMaterial::MetallicRoughnessMaterial(std::shared_ptr<VulkanContext> ctx)
	: m_ctx(ctx)
{

}

MetallicRoughnessMaterial::~MetallicRoughnessMaterial()
{
	vkDestroyPipeline(m_ctx->GetDevice(), m_opaquePipeline.pipeline, nullptr);
	vkDestroyPipeline(m_ctx->GetDevice(), m_transparentPipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(m_ctx->GetDevice(), m_opaquePipeline.layout, nullptr);
	vkDestroyDescriptorSetLayout(m_ctx->GetDevice(), m_materialLayout, nullptr);
}

void MetallicRoughnessMaterial::BuildPipelines(Swapchain& swapchain, VkDescriptorSetLayout gpuSceneDataDescriptorLayout)
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

	GraphicsPipeline pipeline(m_ctx, swapchain);
	pipeline.SetShaders(meshVertexShader, meshFragShader);
	pipeline.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipeline.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipeline.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipeline.SetMultisamplingNone();
	pipeline.DisableBlending();
	pipeline.EnableDepthTest(true);
	pipeline.SetColorAttachmentFormat(swapchain.GetDrawImage().GetFormat());
	pipeline.SetDepthFormat(swapchain.GetDepthImage().GetFormat());
	pipeline.SetLayout(newLayout);
	m_opaquePipeline.pipeline = pipeline.CreateGraphicsPipeline();

	pipeline.EnableBlendingAdditive();
	pipeline.EnableDepthTest(false);
	m_transparentPipeline.pipeline = pipeline.CreateGraphicsPipeline();

	vkDestroyShaderModule(m_ctx->GetDevice(), meshFragShader, nullptr);
	vkDestroyShaderModule(m_ctx->GetDevice(), meshVertexShader, nullptr);
}

MaterialInstance MetallicRoughnessMaterial::WriteMaterial(MaterialPass pass, const MaterialResources& resources, DescriptorAllocator& descriptorAllocator)
{
	MaterialInstance matData;
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
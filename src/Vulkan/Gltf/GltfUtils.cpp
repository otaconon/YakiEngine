#include "GltfUtils.h"

#include <iostream>
#include <vk_mem_alloc.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "../../Ecs.h"
#include "../../Components/Components.h"

std::optional<std::vector<std::shared_ptr<Mesh>>> GltfUtils::load_gltf_meshes(VulkanContext* ctx, const std::filesystem::path& filePath)
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

        if (constexpr bool OverrideColors = false) {
            for (Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal * 0.5f + 0.5f, 1.f);
            }
        }

    	newMesh.meshBuffers = upload_mesh(ctx, vertices, indices);
        meshes.emplace_back(std::make_shared<Mesh>(std::move(newMesh)));
    }

    return meshes;
}

VkFilter GltfUtils::extract_filter(fastgltf::Filter filter)
{
	switch (filter) {
		case fastgltf::Filter::Nearest:
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::NearestMipMapLinear:
			return VK_FILTER_NEAREST;

		case fastgltf::Filter::Linear:
		case fastgltf::Filter::LinearMipMapNearest:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_FILTER_LINEAR;
	}
}
VkSamplerMipmapMode GltfUtils::extract_mipmap_mode(fastgltf::Filter filter)
{
	switch (filter) {
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::LinearMipMapNearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		case fastgltf::Filter::NearestMipMapLinear:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

std::optional<std::shared_ptr<GltfObject>> GltfUtils::load_gltf_object(VulkanContext* ctx, const std::filesystem::path& filePath, Renderer& renderer /*TODO: Remove this parameter*/)
{
	std::println("Loading GLTF file: {}", filePath.string());
	if (!std::filesystem::exists(filePath))
	{
		std::print(std::cerr, "Path doesn't exist: {}", filePath.string());
	}

	std::shared_ptr<GltfObject> scene = std::make_shared<GltfObject>();
	scene->ctx = ctx;
	GltfObject& file = *scene.get();

	constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers;

	fastgltf::Asset gltf;
	fastgltf::Parser parser {};
	auto data = fastgltf::MappedGltfFile::FromPath(filePath);
	auto type = fastgltf::determineGltfFileType(data.get());
	if (type == fastgltf::GltfType::glTF) {
		auto load = parser.loadGltf(data.get(), filePath.parent_path(), gltfOptions);
		if (load) {
			gltf = std::move(load.get());
		} else {
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	} else if (type == fastgltf::GltfType::GLB) {
		auto load = parser.loadGltfBinary(data.get(), filePath.parent_path(), gltfOptions);
		if (load) {
			gltf = std::move(load.get());
		} else {
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	} else {
		std::cerr << "Failed to determine glTF container" << std::endl;
		return {};
	}

	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
	};

	file.descriptorPool.Init(ctx->GetDevice(), gltf.materials.size(), sizes);

	for (fastgltf::Sampler& sampler : gltf.samplers) {

		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
		sampl.maxLod = VK_LOD_CLAMP_NONE;
		sampl.minLod = 0;

		sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		sampl.mipmapMode= extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(ctx->GetDevice(), &sampl, nullptr, &newSampler);

		file.samplers.push_back(newSampler);
	}

	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<Hori::Entity> nodes;
	std::vector<std::shared_ptr<Image>> images;
	std::vector<std::shared_ptr<MaterialInstance>> materials;

	for (fastgltf::Image& image : gltf.images) {
		images.push_back(renderer.errorImage);
	}

	file.materialDataBuffer = std::make_shared<Buffer>(ctx->GetAllocator(), sizeof(MaterialConstants) * gltf.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	int data_index = 0;
	MaterialConstants* sceneMaterialConstants = static_cast<MaterialConstants*>(file.materialDataBuffer->info.pMappedData);

	for (fastgltf::Material& mat : gltf.materials) {
        std::shared_ptr<MaterialInstance> newMat = std::make_shared<MaterialInstance>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        MaterialConstants constants;
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
        constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
        // write material parameters to buffer
        sceneMaterialConstants[data_index] = constants;

        MaterialPass passType = MaterialPass::MainColor;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            passType = MaterialPass::Transparent;
        }

        MaterialResources materialResources;
        // default the material textures
        materialResources.colorImage = renderer.whiteImage;
        materialResources.colorSampler = renderer.defaultSamplerLinear;
        materialResources.metalRoughImage = renderer.whiteImage;
        materialResources.metalRoughSampler = renderer.defaultSamplerLinear;

        // set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer->buffer;
        materialResources.dataBufferOffset = data_index * sizeof(MaterialConstants);
        // grab textures from gltf file
        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            materialResources.colorImage = images[img];
            materialResources.colorSampler = file.samplers[sampler];
        }
        // build material
        *newMat = renderer.GetMetalRoughMaterial().WriteMaterial(passType, materialResources, file.descriptorPool);
        data_index++;
    }

	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	for (fastgltf::Mesh& mesh : gltf.meshes) {
		auto newmesh = std::make_shared<Mesh>();
		meshes.push_back(newmesh);
		file.meshes[mesh.name.c_str()] = newmesh;
		newmesh->name = mesh.name;

		// clear the mesh arrays each mesh, we dont want to merge them by error
		indices.clear();
		vertices.clear();

		for (auto&& p : mesh.primitives) {
			GeoSurface newSurface;
			newSurface.startIndex = (uint32_t)indices.size();
			newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

			size_t initial_vtx = vertices.size();

			// load indexes
			{
				fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + indexaccessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
					[&](std::uint32_t idx) {
						indices.push_back(idx + initial_vtx);
					});
			}

			// load vertex positions
			{
				fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
				vertices.resize(vertices.size() + posAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
					[&](glm::vec3 v, size_t index) {
						Vertex newvtx;
						newvtx.position = v;
						newvtx.normal = { 1, 0, 0 };
						newvtx.color = glm::vec4 { 1.f };
						newvtx.uv_x = 0;
						newvtx.uv_y = 0;
						vertices[initial_vtx + index] = newvtx;
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

			if (p.materialIndex.has_value()) {
				newSurface.material = materials[p.materialIndex.value()];
			} else {
				newSurface.material = materials[0];
			}

			newmesh->surfaces.push_back(newSurface);
		}

		newmesh->meshBuffers = upload_mesh(ctx, vertices, indices);
	}

	// load all nodes and their meshes
	auto& ecs = Ecs::GetInstance();
	for (fastgltf::Node& node : gltf.nodes) {
		Hori::Entity newNode = ecs.CreateEntity();
		ecs.AddComponents(newNode, LocalToWorld{}, LocalToParent{}, ParentToLocal{}, Children{}, Parent{});

		// find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
		if (node.meshIndex.has_value()) {
			Mesh mesh = *meshes[*node.meshIndex].get();
			ecs.AddComponents(newNode, std::move(mesh));
		}

		nodes.push_back(newNode);
		file.nodes[node.name.c_str()];

		auto localTransform = ecs.GetComponent<LocalToWorld>(newNode);
		std::visit(fastgltf::visitor{
			[&]<typename T0>(T0& matrix) {
				// Check if this is the matrix variant
				if constexpr (std::is_same_v<std::decay_t<T0>, fastgltf::Node>) {
					memcpy(&localTransform->value, &matrix.transform, sizeof(matrix.transform));
				}
			},
			[&](fastgltf::TRS& transform) {
				glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
				glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
				glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

				glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
				glm::mat4 rm = glm::toMat4(rot);
				glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

				localTransform->value = tm * rm * sm;
			}
		},
			node.transform
		);
	}

	for (int i = 0; i < gltf.nodes.size(); i++) {
		fastgltf::Node& node = gltf.nodes[i];
		Hori::Entity sceneNode = nodes[i];

		auto children = ecs.GetComponent<Children>(sceneNode);
		for (auto& c : node.children) {
			children->value.push_back(nodes[c]);
			auto parent = ecs.GetComponent<Parent>(nodes[c]);
			parent->value = sceneNode;
		}
	}

	// find the top nodes, with no parents
	for (auto& node : nodes) {
		auto parent = ecs.GetComponent<Parent>(node);
		if (!parent->value.Valid()) {
			file.topNodes.push_back(node);
		}
	}
	return scene;
}

std::shared_ptr<GPUMeshBuffers> GltfUtils::upload_mesh(VulkanContext* ctx, std::vector<Vertex> vertices, std::vector<uint32_t> indices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	Buffer vertexBuffer(ctx->GetAllocator(), vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Buffer indexBuffer(ctx->GetAllocator(), indexBufferSize,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	VkBufferDeviceAddressInfo deviceAddressInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = vertexBuffer.buffer
	};

	std::shared_ptr<GPUMeshBuffers> newSurface = std::make_shared<GPUMeshBuffers> (
		std::move(vertexBuffer),
		std::move(indexBuffer),
		vkGetBufferDeviceAddress(ctx->GetDevice(), &deviceAddressInfo)
		);

	Buffer staging(ctx->GetAllocator(), vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	void* data;
	vmaMapMemory(staging.allocator, staging.allocation, &data);
	if (!data)
		throw std::runtime_error("Failed to allocate staging buffer");

	memcpy(data, vertices.data(), vertexBufferSize);
	memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);
	vmaUnmapMemory(staging.allocator, staging.allocation);

	// TODO: put on a background thread
	ctx->ImmediateSubmit([&](VkCommandBuffer cmd) {
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


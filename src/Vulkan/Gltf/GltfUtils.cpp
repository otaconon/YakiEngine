#include "GltfUtils.h"

#include <vk_mem_alloc.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>

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


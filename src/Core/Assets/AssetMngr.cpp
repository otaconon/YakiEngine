#include "AssetMngr.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <stb_image.h>
#include <glm/gtx/quaternion.hpp>

#include "../../include/YakiEngine/Core/Ecs.h"
#include "../Components/Components.h"
#include "../../include/YakiEngine/App/utils.h"

AssetMngr::AssetMngr(VulkanContext* ctx, MaterialBuilder* materialBuilder)
    : m_ctx{ctx},
	m_materialBuilder{materialBuilder}
{
	initDefaultTextures();
}

AssetMngr::~AssetMngr()
{
	m_deletionQueue.Flush();
}

void AssetMngr::initDefaultTextures()
{
    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    m_defaultTextures.whiteTexture = std::make_shared<Texture>(m_ctx, m_ctx->GetAllocator(), static_cast<void*>(&white), VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16*16> pixels{};
    for (int x = 0; x < 16; x++)
        for (int y = 0; y < 16; y++)
            pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
    m_defaultTextures.errorTexture = std::make_shared<Texture>(m_ctx, m_ctx->GetAllocator(), pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
    m_deletionQueue.PushFunction([this] {
        m_defaultTextures.whiteTexture->Cleanup();
        m_defaultTextures.errorTexture->Cleanup();
    });

    VkSamplerCreateInfo sampler = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(m_ctx->GetDevice(), &sampler, nullptr, &m_defaultTextures.samplerNearest);
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(m_ctx->GetDevice(), &sampler, nullptr, &m_defaultTextures.samplerLinear);

    m_deletionQueue.PushFunction([this]() {
        vkDestroySampler(m_ctx->GetDevice(), m_defaultTextures.samplerNearest, nullptr);
        vkDestroySampler(m_ctx->GetDevice(), m_defaultTextures.samplerLinear, nullptr);
    });
}

std::shared_ptr<Asset> AssetMngr::getAssetImpl(AssetHandle handle)
{
    auto it = m_registry.find(handle);
    if (it == m_registry.end())
    {
        return nullptr;
    }
    return it->second;
}

AssetHandle AssetMngr::registerAssetImpl(std::shared_ptr<Asset> asset)
{
    AssetHandle newHandle{m_currentHandleId++};
    m_registry[newHandle] = asset;
    return newHandle;
}

std::vector<AssetHandle> AssetMngr::loadMeshesImpl(const std::filesystem::path& path)
{
    auto meshes = GltfUtils::load_gltf_meshes(m_ctx, path);
    if (!meshes.has_value())
        return std::vector<AssetHandle>();

    std::vector<AssetHandle> meshHandles;
    meshHandles.reserve(meshes.value().size());
    for (auto mesh : meshes.value())
    {
        AssetHandle handle = RegisterAsset(mesh);
        meshHandles.push_back(handle);
    }

    return meshHandles;
}

std::shared_ptr<GltfObject> AssetMngr::loadGltfImpl(const std::filesystem::path& path)
{
	std::println("Loading GLTF file: {}", path.string());
	if (!std::filesystem::exists(path))
	{
		std::print(std::cerr, "Path doesn't exist: {}", path.string());
		return nullptr;
	}

	std::shared_ptr<GltfObject> scene = std::make_shared<GltfObject>();
	scene->ctx = m_ctx;
	GltfObject& file = *scene.get();

	constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers;

	fastgltf::Asset gltf;
	fastgltf::Parser parser {};
	auto data = fastgltf::MappedGltfFile::FromPath(path);
	auto type = fastgltf::determineGltfFileType(data.get());
	if (type == fastgltf::GltfType::glTF) {
		auto load = parser.loadGltfJson(data.get(), path.parent_path(), gltfOptions);
		if (load) {
			gltf = std::move(load.get());
		} else {
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	} else if (type == fastgltf::GltfType::GLB) {
		auto load = parser.loadGltfBinary(data.get(), path.parent_path(), gltfOptions);
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

	file.descriptorPool.Init(m_ctx->GetDevice(), gltf.materials.size(), sizes);

	for (fastgltf::Sampler& sampler : gltf.samplers) {
		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
		sampl.maxLod = VK_LOD_CLAMP_NONE;
		sampl.minLod = 0;
		sampl.magFilter = GltfUtils::extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		sampl.minFilter = GltfUtils::extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
		sampl.mipmapMode= GltfUtils::extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(m_ctx->GetDevice(), &sampl, nullptr, &newSampler);

		file.samplers.push_back(newSampler);
	}

	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<Hori::Entity> nodes;
	std::vector<std::shared_ptr<Texture>> images;
	std::vector<std::shared_ptr<MaterialInstance>> materials;

	// Load textures
	for (fastgltf::Image& image : gltf.images) {
		std::shared_ptr<Texture> texture = LoadTexture(gltf, image);
		if (texture) {
			RegisterAsset<Texture>(texture);
			images.push_back(texture);
			file.images[image.name.c_str()] = texture;
		}
		else {
			images.push_back(m_defaultTextures.errorTexture);
			std::cout << "gltf failed to load texture " << image.name << std::endl;
		}
	}

	// Load materials
	file.materialDataBuffer = std::make_shared<Buffer>(m_ctx->GetAllocator(), sizeof(MaterialConstants) * gltf.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	int data_index = 0;
	MaterialConstants* sceneMaterialConstants = static_cast<MaterialConstants*>(file.materialDataBuffer->info.pMappedData);
	for (fastgltf::Material &mat : gltf.materials)
	{
        std::shared_ptr<MaterialInstance> newMat = std::make_shared<MaterialInstance>();
        materials.push_back(newMat);
        file.materials[mat.name.c_str()] = newMat;

        MaterialConstants constants {
        	.colorFactors {mat.pbrData.baseColorFactor[0], mat.pbrData.baseColorFactor[1], mat.pbrData.baseColorFactor[2], mat.pbrData.baseColorFactor[3]},
			.metalRoughtFactors {mat.pbrData.metallicFactor, mat.pbrData.roughnessFactor, 0.f, 0.f},
        };
		if (mat.specular)
			constants.specularColorFactors = {mat.specular->specularColorFactor.x(), mat.specular->specularColorFactor.y(), mat.specular->specularColorFactor.z(), mat.specular->specularFactor};

        // write material parameters to buffer
        sceneMaterialConstants[data_index] = constants;

        MaterialPass passType = MaterialPass::MainColor;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            passType = MaterialPass::Transparent;
        }

        MaterialResources materialResources {
        	.colorImage = m_defaultTextures.whiteTexture,
        	.colorSampler = m_defaultTextures.samplerLinear,
        	.metalRoughImage = m_defaultTextures.whiteTexture,
        	.metalRoughSampler = m_defaultTextures.samplerLinear,
        	.dataBuffer = file.materialDataBuffer->buffer,
        	.dataBufferOffset = static_cast<uint32_t>(data_index * sizeof(MaterialConstants))
        };

		// grab textures from gltf file
        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            materialResources.colorImage = images[img];
        	if (images[img]->GetView() == VK_NULL_HANDLE)
        		std::print(std::cerr, "View is null");
            materialResources.colorSampler = file.samplers[sampler];
        }
        // build material
		*newMat = m_materialBuilder->WriteMaterial(passType, materialResources, file.descriptorPool);
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
			newSurface.startIndex = static_cast<uint32_t>(indices.size());
			newSurface.count = static_cast<uint32_t>(gltf.accessors[p.indicesAccessor.value()].count);

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

			// calculate surface bounds
			glm::vec3 minpos = vertices[initial_vtx].position;
			glm::vec3 maxpos = vertices[initial_vtx].position;
			for (int i = initial_vtx; i < vertices.size(); i++) {
				minpos = glm::min(minpos, vertices[i].position);
				maxpos = glm::max(maxpos, vertices[i].position);
			}
			// calculate origin and extents from the min/max, use extent lenght for radius
			newSurface.bounds.origin = (maxpos + minpos) / 2.f;
			newSurface.bounds.extents = (maxpos - minpos) / 2.f;
			newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);

			newmesh->surfaces.push_back(newSurface);
		}

		newmesh->meshBuffers = GltfUtils::upload_mesh(m_ctx, vertices, indices);
	}

	// load all nodes and their meshes
	auto& ecs = Ecs::GetInstance();
	for (fastgltf::Node& node : gltf.nodes) {
		Hori::Entity newNode = ecs.CreateEntity();

		if (node.meshIndex.has_value())
		{
			std::shared_ptr<Mesh> mesh = meshes[*node.meshIndex];
			register_object(newNode, mesh);
		}
		else
		{
			register_object(newNode);
		}

		nodes.push_back(newNode);
		file.nodes[node.name.c_str()] = newNode;


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
				ecs.GetComponent<Translation>(newNode)->value = tl;
				auto rotation = ecs.GetComponent<Rotation>(newNode);
				rotation->value = rot;
				auto euler = glm::eulerAngles(rotation->value);
				rotation->pitch = euler.x;
				rotation->yaw = euler.y;
				rotation->roll = euler.z;
				ecs.GetComponent<Scale>(newNode)->value = sc;
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

	return scene;
}

std::shared_ptr<Texture> AssetMngr::loadTextureImpl(fastgltf::Asset& asset, fastgltf::Image image)
{
	std::shared_ptr<Texture> newTexture {};

    int width, height, nrChannels;

    std::visit(
        fastgltf::visitor {
            [](auto& arg) {},
            [&](fastgltf::sources::URI& filePath) {
                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(filePath.uri.isLocalPath()); // We're only capable of loading

                const std::string path(filePath.uri.path().begin(), filePath.uri.path().end()); // Thanks C++.
                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newTexture = std::make_shared<Texture>(m_ctx, m_ctx->GetAllocator(), data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Vector& vector) {
                unsigned char* data = stbi_load_from_memory(bit_cast<stbi_uc*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newTexture = std::make_shared<Texture>(m_ctx, m_ctx->GetAllocator(), data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::BufferView& view) {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];

                std::visit(fastgltf::visitor {
                	[](auto& arg) {
                		std::println(std::cerr, "Unhandled buffer data type");
                	},
					[&](fastgltf::sources::Array& vector) {
					   stbi_uc* data = stbi_load_from_memory(bit_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset), static_cast<int>(bufferView.byteLength), &width, &height, &nrChannels, 4);
					   if (data) {
						   VkExtent3D imagesize;
						   imagesize.width = width;
						   imagesize.height = height;
						   imagesize.depth = 1;

						   newTexture = std::make_shared<Texture>(m_ctx, m_ctx->GetAllocator(), data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

						   stbi_image_free(data);
					   }
				   } },
				   buffer.data);
            },
        },
        image.data);

    // if any of the attempts to load the data failed, we havent written the image
    // so handle is null
    if (!newTexture || newTexture->GetImage() == VK_NULL_HANDLE) {
        return {};
    } else {
        return newTexture;
    }
}

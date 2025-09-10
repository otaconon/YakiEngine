#include <Assets/Scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <stb_image.h>
#include <glm/gtx/quaternion.hpp>

#include <print>
#include <filesystem>

#include "Ecs.h"
#include "Components/CoreComponents.h"
#include "Assets/AssetHandle.h"
#include "Assets/AssetMngr.h"
#include "Assets/GltfUtils.h"
#include "Assets/Material.h"
#include "Assets/ShaderEffect.h"
#include "Assets/utils.h"
#include "Components/DefaultData.h"
#include "Vulkan/VkTypes.h"
#include "Vulkan/Descriptors/DescriptorWriter.h"

Scene::Scene(VulkanContext *ctx, const std::filesystem::path &path)
  : m_ctx{ctx} {
  std::println("Loading GLTF file: {}", path.string());
  if (!std::filesystem::exists(path)) {
    std::print(std::cerr, "Path doesn't exist: {}", path.string());
    return;
  }

  constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers;

  fastgltf::Asset gltf;
  fastgltf::Parser parser{};
  auto data = fastgltf::MappedGltfFile::FromPath(path);
  auto type = fastgltf::determineGltfFileType(data.get());
  if (type == fastgltf::GltfType::glTF) {
    auto load = parser.loadGltfJson(data.get(), path.parent_path(), gltfOptions);
    if (load) {
      gltf = std::move(load.get());
    } else {
      std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
      return;
    }
  } else if (type == fastgltf::GltfType::GLB) {
    auto load = parser.loadGltfBinary(data.get(), path.parent_path(), gltfOptions);
    if (load) {
      gltf = std::move(load.get());
    } else {
      std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
      return;
    }
  } else {
    std::cerr << "Failed to determine glTF container" << std::endl;
    return;
  }

  std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
  };

  m_descriptorPool.Init(m_ctx->GetDevice(), gltf.materials.size(), sizes);

  for (fastgltf::Sampler &sampler : gltf.samplers) {
    VkSamplerCreateInfo samplerCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .magFilter = GltfUtils::extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
      .minFilter = GltfUtils::extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
      .mipmapMode = GltfUtils::extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
      .minLod = 0,
      .maxLod = VK_LOD_CLAMP_NONE
    };

    VkSampler newSampler;
    vkCreateSampler(m_ctx->GetDevice(), &samplerCreateInfo, nullptr, &newSampler);
    m_samplers.push_back(newSampler);
  }

  std::vector<std::shared_ptr<Mesh>> meshes;
  std::vector<Hori::Entity> nodes;
  std::vector<std::shared_ptr<Texture>> images;
  std::vector<std::shared_ptr<Material>> materials;

  DefaultData* defaultData = Ecs::GetInstance().GetSingletonComponent<DefaultData>();
  for (fastgltf::Image &image : gltf.images) {
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(gltf, image);
    if (texture->GetImage()) {
      AssetMngr::RegisterAsset<Texture>(texture);
      images.push_back(texture);
      m_images[image.name.c_str()] = texture;
    } else {
      images.push_back(defaultData->errorTexture);
      std::cout << "gltf failed to load texture " << image.name << std::endl;
    }
  }

  m_materialDataBuffer = std::make_shared<Buffer>(m_ctx->GetAllocator(), sizeof(ShaderParameters) * gltf.materials.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
  int data_index = 0;
  ShaderParameters *shaderParams = static_cast<ShaderParameters *>(m_materialDataBuffer->info.pMappedData);
  for (fastgltf::Material &mat : gltf.materials) {
    auto newMat = std::make_shared<Material>();
    materials.push_back(newMat);
    m_materials[mat.name.c_str()] = newMat;

    newMat->parameters = {
      .colorFactors{mat.pbrData.baseColorFactor[0], mat.pbrData.baseColorFactor[1], mat.pbrData.baseColorFactor[2], mat.pbrData.baseColorFactor[3]},
      .metalRoughFactors{mat.pbrData.metallicFactor, mat.pbrData.roughnessFactor, 0.f, 0.f}
    };

    if (mat.specular)
      newMat->parameters.specularColorFactors = {mat.specular->specularColorFactor.x(), mat.specular->specularColorFactor.y(), mat.specular->specularColorFactor.z(), mat.specular->specularFactor};

    // write material parameters to buffer
    shaderParams[data_index] = newMat->parameters;

    TransparencyMode passType = TransparencyMode::Opaque;
    if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
      passType = TransparencyMode::Transparent;
    }

    struct MaterialResources {
      std::shared_ptr<Texture> colorImage;
      VkSampler colorSampler;
      std::shared_ptr<Texture> metalRoughImage;
      VkSampler metalRoughSampler;
      VkBuffer dataBuffer;
      uint32_t dataBufferOffset;
    };

    MaterialResources materialResources {
        .colorImage = defaultData->errorTexture,
        .colorSampler = defaultData->samplerLinear,
        .metalRoughImage = defaultData->errorTexture,
        .metalRoughSampler = defaultData->samplerLinear,
        .dataBuffer = m_materialDataBuffer->buffer,
        .dataBufferOffset = static_cast<uint32_t>(data_index * sizeof(ShaderParameters))
    };

    if (mat.pbrData.baseColorTexture.has_value()) {
      size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
      size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

      materialResources.colorImage = images[img];
      if (images[img]->GetView() == VK_NULL_HANDLE)
        std::print(std::cerr, "View is null");
      materialResources.colorSampler = m_samplers[sampler];
    }

    newMat->original = defaultData->opaqueEffectTemplate;
    DescriptorAllocator descriptorAllocator{};
    newMat->passSets[MeshPassType::Forward] = descriptorAllocator.Allocate(m_ctx->GetDevice(), newMat->original->passShaders[MeshPassType::Forward]->effect->descriptorSetLayouts[1]); // Possibly needs a fix here

    DescriptorWriter writer{};
    writer.Clear();
    writer.WriteBuffer(0, materialResources.dataBuffer, sizeof(ShaderParameters), materialResources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.WriteImage(1, materialResources.colorImage->GetView(), materialResources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.WriteImage(2, materialResources.metalRoughImage->GetView(), materialResources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.UpdateSet(m_ctx->GetDevice(), newMat->passSets[MeshPassType::Forward]);

    data_index++;
  }

  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;

  for (fastgltf::Mesh &mesh : gltf.meshes) {
    auto newmesh = std::make_shared<Mesh>();
    meshes.push_back(newmesh);
    m_meshes[mesh.name.c_str()] = newmesh;
    newmesh->name = mesh.name;

    indices = {};
    vertices = {};

    for (auto &&p : mesh.primitives) {
      GeoSurface newSurface;
      newSurface.startIndex = static_cast<uint32_t>(indices.size());
      newSurface.count = static_cast<uint32_t>(gltf.accessors[p.indicesAccessor.value()].count);

      size_t initial_vtx = vertices.size();

      // load indexes
      {
        fastgltf::Accessor &indexaccessor = gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor, [&](std::uint32_t idx) {
          indices.push_back(idx + initial_vtx);
        });
      }

      // load vertex positions
      {
        fastgltf::Accessor &posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index) {
          Vertex newvtx;
          newvtx.position = v;
          newvtx.normal = {1, 0, 0};
          newvtx.color = glm::vec4{1.f};
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
    newmesh->indices = std::move(indices);
    newmesh->vertices = std::move(vertices);
  }

  // load all nodes and their meshes
  auto &ecs = Ecs::GetInstance();
  for (fastgltf::Node &node : gltf.nodes) {
    Hori::Entity newNode = ecs.CreateEntity();

    if (node.meshIndex.has_value()) {
      std::shared_ptr<Mesh> mesh = meshes[*node.meshIndex];
      register_object(newNode, mesh);
    } else {
      register_object(newNode);
    }

    nodes.push_back(newNode);
    m_nodes[node.name.c_str()] = newNode;

    auto localTransform = ecs.GetComponent<LocalToWorld>(newNode);
    std::visit(fastgltf::visitor{
            [&]<typename T0>(T0 &matrix) {
              // Check if this is the matrix variant
              if constexpr (std::is_same_v<std::decay_t<T0>, fastgltf::Node>) {
                memcpy(&localTransform->value, &matrix.transform, sizeof(matrix.transform));
              }
            },
            [&](fastgltf::TRS &transform) {
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
    fastgltf::Node &node = gltf.nodes[i];
    Hori::Entity sceneNode = nodes[i];

    auto children = ecs.GetComponent<Children>(sceneNode);
    for (auto &c : node.children) {
      children->value.push_back(nodes[c]);
      auto parent = ecs.GetComponent<Parent>(nodes[c]);
      parent->value = sceneNode;
    }
  }
}

Scene::~Scene() {
  for (auto &sampler : m_samplers) {
    vkDestroySampler(m_ctx->GetDevice(), sampler, nullptr);
  }

  m_descriptorPool.DestroyPools(m_ctx->GetDevice());
}
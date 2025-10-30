#pragma once

#include "Mesh.h"
#include "ShaderEffect.h"
#include "Texture.h"
#include "TextureManager.h"

#include <memory>
#include <filesystem>
#include <HECS/Core/World.h>

class HashCubes;

class Scene {
public:
  Scene(std::shared_ptr<VulkanContext> ctx, DeletionQueue& deletionQueue, const std::filesystem::path& path, TextureManager& textureManager);
  ~Scene();

  void Instantiate();

  std::vector<std::shared_ptr<Mesh>> meshes;
  std::vector<AssetHandle<Texture>> m_textures;

private:
  friend HashCubes; // TODO: Remove this line

  fastgltf::Asset m_gltf;
  std::shared_ptr<VulkanContext> m_ctx;

  std::vector<Hori::Entity> m_nodes;
  std::vector<std::shared_ptr<Material>> m_materials;

  std::vector<VkSampler> m_samplers;

  TextureManager m_textureManager;

  DescriptorAllocator m_descriptorAllocator;
};
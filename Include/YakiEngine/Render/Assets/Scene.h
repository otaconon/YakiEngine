#pragma once

#include "Mesh.h"
#include "ShaderEffect.h"
#include "Texture.h"

#include <iostream>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <print>
#include <HECS/Core/World.h>

class HashCubes;

class Scene {
public:
  Scene(std::shared_ptr<VulkanContext> ctx, DeletionQueue& deletionQueue, const std::filesystem::path& path);
  ~Scene();

  std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
private:
  friend HashCubes; // TODO: Remove this line

  std::shared_ptr<VulkanContext> m_ctx;

  std::unordered_map<std::string, Hori::Entity> m_nodes;
  std::unordered_map<std::string, std::shared_ptr<Texture>> m_images;
  std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;

  std::vector<VkSampler> m_samplers;

  DescriptorAllocator m_descriptorAllocator;
  std::shared_ptr<Buffer> m_materialDataBuffer;
};
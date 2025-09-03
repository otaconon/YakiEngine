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

class Scene {
public:
  Scene(VulkanContext* ctx, const std::filesystem::path& path);
  ~Scene();

private:
  VulkanContext* m_ctx;

  std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
  std::unordered_map<std::string, Hori::Entity> m_nodes;
  std::unordered_map<std::string, std::shared_ptr<Texture>> m_images;
  std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;

  std::vector<VkSampler> m_samplers;

  DescriptorAllocator m_descriptorPool;
  std::shared_ptr<Buffer> m_materialDataBuffer;
};
#pragma once


#include "Vulkan/Renderer.h"
#include "SDL/Window.h"

#include "Assets/AssetMngr.h"
#include "Assets/GltfUtils.h"
#include "Assets/Scene.h"

#include "Vulkan/VulkanContext.h"

class HashCubes {
public:
  HashCubes();
  ~HashCubes();

  void Run();

private:
  Window m_window{};
  VulkanContext m_ctx;
  Renderer m_renderer;

  DeletionQueue m_deletionQueue;

  std::shared_ptr<Scene> m_allMeshes;
  std::shared_ptr<Mesh> m_cubeMesh;

private:
  void initEcs();
};
#pragma once

#include <string>
#include <vector>

#include "../../Core/Assets/Asset.h"
#include "Vulkan/VkTypes.h"

struct Mesh : Asset {
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<GeoSurface> surfaces;
  std::shared_ptr<GPUMeshBuffers> meshBuffers;
};

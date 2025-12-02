#pragma once

#include "GeoSurface.h"

#include <string>
#include <vector>

#include "Vulkan/VkTypes.h"

struct Mesh {
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<GeoSurface> surfaces;
  std::shared_ptr<GPUMeshBuffers> meshBuffers;
};

#pragma once

#include <glm/glm.hpp>
#include "Material.h"

struct Bounds {
  glm::vec3 origin;
  float sphereRadius;
  glm::vec3 extents;
};

struct GeoSurface {
  uint32_t startIndex;
  uint32_t count;
  Bounds bounds;
  std::shared_ptr<Material> material;
};

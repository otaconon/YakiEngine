#pragma once
#include <memory>

#include "Assets/Mesh.h"

struct DynamicObject {
  std::shared_ptr<Mesh> mesh;
  glm::mat4 transform;
};
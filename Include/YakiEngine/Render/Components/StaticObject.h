#pragma once
#include <memory>
#include <glm/glm.hpp>

#include "Assets/Mesh.h"

struct StaticObject {
  std::shared_ptr<Mesh> mesh;
  glm::mat4 transform;
};

// Used to tag static objects that need an update
struct DirtyStaticObject {
  
};
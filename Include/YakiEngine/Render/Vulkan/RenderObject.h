#pragma once

#include <cstdint>

#include "VkTypes.h"
#include "Assets/Mesh.h"

struct RenderObject
{
    uint32_t objectId;

    uint32_t indexCount;
    uint32_t firstIndex;
    VkBuffer indexBuffer;

    Mesh* mesh;
    Material* material;
    Bounds bounds;

    glm::mat4 transform;
    VkDeviceAddress vertexBufferAddress;
};

struct RenderIndirectObjects {
  std::vector<uint32_t> firstIndices;
  std::vector<uint32_t> indexCounts;
  std::vector<uint32_t> vertexOffsets;
  std::vector<uint32_t> objectIds;
  std::vector<glm::mat4> transforms;
  std::vector<Mesh *> meshes;
  std::vector<Material *> materials;
};

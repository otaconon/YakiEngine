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
    MaterialInstance* material;
    Bounds bounds;

    glm::mat4 transform;
    VkDeviceAddress vertexBufferAddress;
};

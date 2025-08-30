#pragma once

#include <string>
#include <vector>

#include "Asset.h"
#include "Vulkan/VkTypes.h"

struct Mesh : Asset
{
    std::string name;
    std::vector<GeoSurface> surfaces;
    std::shared_ptr<GPUMeshBuffers> meshBuffers;
};

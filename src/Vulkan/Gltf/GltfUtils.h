#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "../VulkanContext.h"
#include "../Buffer.h"

namespace GltfUtils
{
    [[nodiscard]] std::optional<std::vector<std::shared_ptr<Mesh>>> load_gltf_meshes(VulkanContext* ctx, const std::filesystem::path& filePath);
    [[nodiscard]] std::shared_ptr<GPUMeshBuffers> upload_mesh(VulkanContext* ctx, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
}

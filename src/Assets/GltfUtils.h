#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>
#include <fastgltf/core.hpp>

#include "../Vulkan/VulkanContext.h"
#include "../Vulkan/Buffer.h"
#include "GltfObject.h"
#include "../Vulkan/MetallicRoughnessMaterial.h"
#include "../Vulkan/Renderer.h"
#include "Mesh.h"

namespace GltfUtils
{
    [[nodiscard]] std::optional<std::vector<std::shared_ptr<Mesh>>> load_gltf_meshes(VulkanContext* ctx, const std::filesystem::path& filePath);
    [[nodiscard]] VkFilter extract_filter(fastgltf::Filter filter);
    [[nodiscard]] VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter);
    [[nodiscard]] std::optional<std::shared_ptr<GltfObject>> load_gltf_object(VulkanContext* ctx, const std::filesystem::path& filePath, Renderer& renderer, DefaultData& defaultData);
    [[nodiscard]] std::shared_ptr<GPUMeshBuffers> upload_mesh(VulkanContext* ctx, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
}

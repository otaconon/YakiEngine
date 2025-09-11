#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/types.hpp>

#include "Vulkan/VulkanContext.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Renderer.h"
#include "Assets/Mesh.h"

namespace GltfUtils
{
    [[nodiscard]] std::optional<std::vector<std::shared_ptr<Mesh>>> load_gltf_meshes(std::shared_ptr<VulkanContext> ctx, const std::filesystem::path& filePath);

    [[nodiscard]] std::shared_ptr<GPUMeshBuffers> upload_mesh(std::shared_ptr<VulkanContext> ctx, std::vector<Vertex> vertices, std::vector<uint32_t> indices);

    [[nodiscard]] VkFilter extract_filter(fastgltf::Filter filter);
    [[nodiscard]] VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter);
}

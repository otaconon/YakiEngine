#pragma once

#include <HECS/Entity.h>
#include <filesystem>
#include <fstream>
#include <print>

#include "DefaultData.h"
#include "Ecs.h"
#include "PropertyRegistry.h"
#include "Assets/Mesh.h"
#include "Components/Components.h"

inline std::vector<char> read_file(const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath))
    {
        std::print("Failed to read file, path {} doesn't exist", std::filesystem::absolute(filepath).string());
        return {};
    }

    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::print("Failed to read file, couldn't open path {}", std::filesystem::absolute(filepath).string());
        return {};
    }

    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

[[nodiscard]] inline Drawable create_drawable(std::shared_ptr<Mesh> mesh)
{
    Drawable drawable{mesh, {}, {}, {glm::mat4{1.f}, glm::mat4{}, glm::mat4{}}};
    return drawable;
}

inline void register_object(Hori::Entity e, std::shared_ptr<Mesh> mesh, Translation pos)
{
    auto& ecs = Ecs::GetInstance();
    ecs.AddComponents(e, create_drawable(mesh), std::move(pos), Rotation{}, Scale{{1.f, 1.f, 1.f}}, BoxCollider{{0.5f, 0.5f, 0.5f}, true});
    register_property<Translation>(e, "Translation");
    register_property<Rotation>(e, "Rotation");
    register_property<Scale>(e, "Scale");
}
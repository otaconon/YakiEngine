#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <print>

#include "Asset.h"
#include "AssetHandle.h"
#include "Mesh.h"
#include "Vulkan/VulkanContext.h"
#include "MaterialBuilder.h"
#include "Assets/GltfUtils.h"
#include "Assets/Texture.h"

class AssetMngr
{
public:
    static void Initialize(VulkanContext* ctx, MaterialBuilder* materialBuilder)
    {
        s_instance = new AssetMngr(ctx, materialBuilder);
    }

    static void Shutdown()
    {
        delete s_instance;
        s_instance = nullptr;
    }

    template<typename AssetType>
    [[nodiscard]] static std::shared_ptr<AssetType> GetAsset(AssetHandle handle)
    {
        return std::dynamic_pointer_cast<AssetType>(s_instance->getAssetImpl(handle));
    }

    template<typename AssetType>
    static AssetHandle RegisterAsset(std::shared_ptr<AssetType> asset)
    {
        return s_instance->registerAssetImpl(asset);
    }

    [[nodiscard]] static std::vector<AssetHandle> LoadMeshes(const std::filesystem::path& path)
    {
        return s_instance->loadMeshesImpl(path);
    }

    [[nodiscard]] static std::shared_ptr<GltfObject> LoadGltf(const std::filesystem::path& path)
    {
        return s_instance->loadGltfImpl(path);
    }

    [[nodiscard]] static std::shared_ptr<Texture> LoadTexture(fastgltf::Asset& asset, fastgltf::Image image)
    {
        return s_instance->loadTextureImpl(asset, image);
    }

private:
    inline static AssetMngr* s_instance;

    VulkanContext* m_ctx;
    uint32_t m_currentHandleId{0};
    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_registry;

    DeletionQueue m_deletionQueue;

    DefaultTextures m_defaultTextures;
    MaterialBuilder* m_materialBuilder;

private:
    explicit AssetMngr(VulkanContext* ctx, MaterialBuilder* materialBuilder);
    ~AssetMngr();
    AssetMngr(const AssetMngr&) = default;
    AssetMngr& operator=(const AssetMngr&) = default;

    void initDefaultTextures();

    [[nodiscard]] std::shared_ptr<Asset> getAssetImpl(AssetHandle handle);
    [[nodiscard]] AssetHandle registerAssetImpl(std::shared_ptr<Asset> asset);
    [[nodiscard]] std::vector<AssetHandle> loadMeshesImpl(const std::filesystem::path& path);
    [[nodiscard]] std::shared_ptr<GltfObject> loadGltfImpl(const std::filesystem::path& path);
    [[nodiscard]] std::shared_ptr<Texture> loadTextureImpl(fastgltf::Asset& asset, fastgltf::Image image);
};

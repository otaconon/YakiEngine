#pragma once
#include <filesystem>
#include <memory>
#include <unordered_map>

#include "Asset.h"
#include "AssetHandle.h"
#include "Mesh.h"
#include "../Vulkan/VulkanContext.h"

class AssetMngr
{
public:
    explicit AssetMngr(VulkanContext* ctx);

    static void Initialize(VulkanContext* ctx)
    {
        s_instance = new AssetMngr(ctx);
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
    [[nodiscard]] static AssetHandle RegisterAsset(std::shared_ptr<AssetType> asset)
    {
        return s_instance->registerAssetImpl(asset);
    }

    [[nodiscard]] static std::vector<AssetHandle> LoadMeshes(const std::filesystem::path& path)
    {
        return s_instance->loadMeshesImpl(path);
    }

private:
    inline static AssetMngr* s_instance;

    VulkanContext* m_ctx;
    uint32_t m_currentHandleId{0};
    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_registry;

private:
    [[nodiscard]] std::shared_ptr<Asset> getAssetImpl(AssetHandle handle);
    [[nodiscard]] AssetHandle registerAssetImpl(std::shared_ptr<Asset> asset);
    [[nodiscard]] std::vector<AssetHandle> loadMeshesImpl(const std::filesystem::path& path);
};

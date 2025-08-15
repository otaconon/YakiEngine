#include "AssetMngr.h"

#include "GltfUtils.h"


AssetMngr::AssetMngr(VulkanContext* ctx)
    : m_ctx{ctx}
{}

std::shared_ptr<Asset> AssetMngr::getAssetImpl(AssetHandle handle)
{
    auto it = m_registry.find(handle);
    if (it == m_registry.end())
    {
        return nullptr;
    }
    return it->second;
}

AssetHandle AssetMngr::registerAssetImpl(std::shared_ptr<Asset> asset)
{
    AssetHandle newHandle{m_currentHandleId++};
    m_registry[newHandle] = asset;
    return newHandle;
}

std::vector<AssetHandle> AssetMngr::loadMeshesImpl(const std::filesystem::path& path)
{
    auto meshes = GltfUtils::load_gltf_meshes(m_ctx, path);
    if (!meshes.has_value())
        return std::vector<AssetHandle>();

    std::vector<AssetHandle> meshHandles;
    meshHandles.reserve(meshes.value().size());
    for (auto mesh : meshes.value())
    {
        AssetHandle handle = RegisterAsset(mesh);
        meshHandles.push_back(handle);
    }

    return meshHandles;
}

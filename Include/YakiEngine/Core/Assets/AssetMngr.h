#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <print>

#include "Assets/Asset.h"
#include "Assets/AssetHandle.h"

class AssetMngr
{
public:
    static AssetMngr& GetInstance() noexcept {
      static AssetMngr instance;
      return instance;
    }

    template<typename AssetType>
    [[nodiscard]] static std::shared_ptr<AssetType> GetAsset(AssetHandle handle)
    {
        return std::dynamic_pointer_cast<AssetType>(GetInstance().getAssetImpl(handle));
    }

    template<typename AssetType>
    static AssetHandle RegisterAsset(std::shared_ptr<AssetType> asset)
    {
        return GetInstance().registerAssetImpl(asset);
    }

private:
    uint32_t m_currentHandleId{0};
    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_registry;

private:
    AssetMngr() = default;
    ~AssetMngr();

    void initDefaultTextures();

    [[nodiscard]] std::shared_ptr<Asset> getAssetImpl(AssetHandle handle);
    [[nodiscard]] AssetHandle registerAssetImpl(std::shared_ptr<Asset> asset);
};

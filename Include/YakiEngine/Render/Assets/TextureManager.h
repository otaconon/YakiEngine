#pragma once
#include "Texture.h"
#include "Assets/AssetHandle.h"

#include <memory>
#include <vector>

class TextureManager {
public:
  TextureManager() = default;

  AssetHandle<Texture> RegisterTexture(std::shared_ptr<Texture> texture) {
    for (uint32_t i = 0; i < id2tex.size(); i++) {
      if (id2tex[i] == nullptr) {
        id2tex[i] = texture;
        return AssetHandle<Texture>(i);
      }
    }
    id2tex.push_back(texture);
    return AssetHandle<Texture>(static_cast<uint32_t>(id2tex.size()-1));
  }

  std::shared_ptr<Texture> GetTexture(AssetHandle<Texture> handle) {
    if (handle.id >= id2tex.size() || id2tex[handle.id] == nullptr) {
      return nullptr;
    }
    return id2tex[handle.id];
  }

private:
  std::vector<std::shared_ptr<Texture>> id2tex;
};
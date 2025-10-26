#pragma once
#include "Texture.h"

#include <memory>
#include <vector>

class TextureManager {
public:
  TextureManager() = default;

  uint32_t RegisterTexture(std::shared_ptr<Texture> texture) {
    for (uint32_t i = 0; i < id2tex.size(); i++) {
      if (id2tex[i] == nullptr) {
        id2tex[i] = texture;
        return i;
      }
    }
    id2tex.push_back(texture);
    return static_cast<uint32_t>(id2tex.size());
  }

private:
  std::vector<std::shared_ptr<Texture>> id2tex;
};
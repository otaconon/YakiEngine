#include "Assets/AssetMngr.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <stb_image.h>
#include <glm/gtx/quaternion.hpp>

#include "Ecs.h"
#include "Components/CoreComponents.h"
#include "Assets/AssetHandle.h"

// TODO: Uncomment this and move from here
/*
void AssetMngr::initDefaultTextures() {

}
*/

std::shared_ptr<Asset> AssetMngr::getAssetImpl(AssetHandle handle) {
  auto it = m_registry.find(handle);
  if (it == m_registry.end()) {
    return nullptr;
  }
  return it->second;
}

AssetHandle AssetMngr::registerAssetImpl(std::shared_ptr<Asset> asset) {
  AssetHandle newHandle{m_currentHandleId++};
  m_registry[newHandle] = asset;
  return newHandle;
}
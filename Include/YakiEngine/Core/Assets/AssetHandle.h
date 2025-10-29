#pragma once

#include <cstdint>
#include <functional>

template<typename T>
struct AssetHandle {
  uint32_t id{0};

  bool Valid() const {
    return id != 0;
  }

  bool operator==(const AssetHandle &other) const {
    return id == other.id;
  }

  bool operator<(const AssetHandle &other) const {
    return id < other.id;
  }
};

template <typename T>
struct std::hash<AssetHandle<T>> {
  std::size_t operator()(const AssetHandle<T> &handle) const noexcept {
    return std::hash<uint32_t>{}(handle.id);
  }
};
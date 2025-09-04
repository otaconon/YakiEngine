#pragma once

#include <array>
#include <type_traits>

template<typename T>
concept EnumType = std::is_enum_v<T>;

template<typename T, EnumType E, size_t Size>
class EnumAccessArray {
public:
  T &operator[](E type) {
    return shaderPasses[static_cast<size_t>(type)];
  }

  const T &operator[](E type) const {
    return shaderPasses[static_cast<size_t>(type)];
  }

private:
  std::array<T, Size> shaderPasses{};
};

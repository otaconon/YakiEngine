#pragma once

#include <cstdint>
#include <functional>

struct AssetHandle
{
    uint32_t id{0};

    bool Valid() const
    {
        return id != 0;
    }

    bool operator==(const AssetHandle& other) const
    {
        return id == other.id;
    }
};

template<>
struct std::hash<AssetHandle>
{
    std::size_t operator()(const AssetHandle& handle) const noexcept
    {
        return std::hash<uint32_t>{}(handle.id);
    }
};
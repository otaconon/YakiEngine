#pragma once

#include <filesystem>
#include <fstream>
#include <print>

static std::vector<char> readFile(const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath))
    {
        std::print("Failed to read file, path {} doesn't exist", std::filesystem::absolute(filepath).string());
        return {};
    }

    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::print("Failed to read file, couldn't open path {}", std::filesystem::absolute(filepath).string());
        return {};
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}
#pragma once

#include "DescriptorAllocator.h"
#include "../VkCheck.h"

class DescriptorLayoutBuilder
{
public:
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    DescriptorLayoutBuilder();
    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
    void AddBinding(uint32_t binding, VkDescriptorType type);
    void Clear();
};

#include "DescriptorLayoutBuilder.h"

DescriptorLayoutBuilder::DescriptorLayoutBuilder() = default;

void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newbind {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1,
    };

    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::Clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : bindings)
        b.stageFlags |= shaderStages;

    VkDescriptorSetLayoutCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = pNext,
        .flags = flags,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}

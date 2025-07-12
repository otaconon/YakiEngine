#pragma once

#include <deque>
#include <vector>
#include <vulkan/vulkan.h>

#include "../VkCheck.h"

class DescriptorWriter {
public:
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    DescriptorWriter();

    void WriteImage(uint32_t binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void WriteBuffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void Clear();
    void UpdateSet(VkDevice device, VkDescriptorSet set);
};

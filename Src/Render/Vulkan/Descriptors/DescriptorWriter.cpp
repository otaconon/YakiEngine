#include "Vulkan/Descriptors/DescriptorWriter.h"

DescriptorWriter::DescriptorWriter() = default;

void DescriptorWriter::WriteImage(uint32_t binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = image,
        .imageLayout = layout
    });

    VkWriteDescriptorSet write {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &info
    };

    writes.push_back(write);
}

void DescriptorWriter::WriteBuffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = offset,
        .range = size
    });

    VkWriteDescriptorSet write {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &info
    };

    writes.push_back(write);
}

void DescriptorWriter::Clear()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : writes)
        write.dstSet = set;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

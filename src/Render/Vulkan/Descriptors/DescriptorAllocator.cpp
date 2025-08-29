#include "Vulkan/Descriptors/DescriptorAllocator.h"

DescriptorAllocator::DescriptorAllocator() = default;

void DescriptorAllocator::Init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    m_ratios.clear();
    for (auto r : poolRatios) {
        m_ratios.push_back(r);
    }

    VkDescriptorPool newPool = createPool(device, maxSets, poolRatios);
    m_setsPerPool = maxSets * 1.5;
    m_readyPools.push_back(newPool);
}

void DescriptorAllocator::ClearPools(VkDevice device)
{
    for (auto p : m_readyPools)
        vkResetDescriptorPool(device, p, 0);

    for (auto p : m_fullPools) {
        vkResetDescriptorPool(device, p, 0);
        m_readyPools.push_back(p);
    }

    m_fullPools.clear();
}

void DescriptorAllocator::DestroyPools(VkDevice device)
{
    for (auto p : m_readyPools)
        vkDestroyDescriptorPool(device, p, nullptr);
    m_readyPools.clear();

    for (auto p : m_fullPools)
        vkDestroyDescriptorPool(device,p,nullptr);
    m_fullPools.clear();
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
    VkDescriptorPool poolToUse = getPool(device);

    VkDescriptorSetAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = pNext,
        .descriptorPool = poolToUse,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        m_fullPools.push_back(poolToUse);
        poolToUse = getPool(device);
        allocInfo.descriptorPool = poolToUse;
        VK_CHECK( vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }

    m_readyPools.push_back(poolToUse);
    return ds;
}

VkDescriptorPool DescriptorAllocator::getPool(VkDevice device)
{
    VkDescriptorPool newPool;
    if (!m_readyPools.empty()) {
        newPool = m_readyPools.back();
        m_readyPools.pop_back();
    }
    else {
        newPool = createPool(device, m_setsPerPool, m_ratios);

        m_setsPerPool = m_setsPerPool * 1.5;
        if (m_setsPerPool > 4092)
            m_setsPerPool = 4092;
    }

    return newPool;
}
VkDescriptorPool DescriptorAllocator::createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = static_cast<uint32_t>(ratio.ratio * setCount)
        });
    }

    VkDescriptorPoolCreateInfo pool_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = setCount,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
    return newPool;
}

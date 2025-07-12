#pragma once

#include <vulkan/vulkan.h>
#include <span>
#include <vector>

#include "../VkCheck.h"

class DescriptorAllocator {
public:
	struct PoolSizeRatio {
		VkDescriptorType type;
		float ratio;
	};

	DescriptorAllocator();
	void Init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
	void ClearPools(VkDevice device);
	void DestroyPools(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);

private:
	VkDescriptorPool getPool(VkDevice device);
	VkDescriptorPool createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

	std::vector<PoolSizeRatio> m_ratios;
	std::vector<VkDescriptorPool> m_fullPools;
	std::vector<VkDescriptorPool> m_readyPools;
	uint32_t m_setsPerPool;
};

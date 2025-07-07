#pragma once
#include <vk_mem_alloc.h>

#include "VkUtils.h"

struct Buffer
{
    Buffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
        : allocator(allocator)
    {
        VkBufferCreateInfo bufferInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = allocSize,
            .usage = usage
        };

        VmaAllocationCreateInfo vmaAllocInfo {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = memoryUsage
        };

        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &buffer, &allocation, &info));
    }

    Buffer(Buffer&& other) noexcept
        : buffer(other.buffer)
        , allocator(other.allocator)
        , allocation(other.allocation)
        , info(other.info)
    {
        other.buffer = VK_NULL_HANDLE;
        other.allocation = nullptr;
        other.allocator = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept
    {
        if (this != &other) {
            if (buffer != VK_NULL_HANDLE && allocation != nullptr && allocator != nullptr) {
                vmaDestroyBuffer(allocator, buffer, allocation);
            }

            buffer = other.buffer;
            allocator = other.allocator;
            allocation = other.allocation;
            info = other.info;

            other.buffer = VK_NULL_HANDLE;
            other.allocation = nullptr;
            other.allocator = nullptr;
        }
        return *this;
    }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    ~Buffer()
    {
        if (buffer != VK_NULL_HANDLE && allocation != nullptr && allocator != nullptr) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo info{};
};



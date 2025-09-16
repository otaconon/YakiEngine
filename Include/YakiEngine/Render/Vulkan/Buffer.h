#pragma once

#include "VkCheck.h"

#include <vk_mem_alloc.h>

struct Buffer {
  Buffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    : allocator(allocator) {
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = allocSize,
        .usage = usage
    };

    VmaAllocationCreateInfo vmaAllocInfo{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memoryUsage
    };

    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &buffer, &allocation, &info));
  }

  Buffer(Buffer &&other) noexcept
    : buffer(other.buffer)
      , allocator(other.allocator)
      , allocation(other.allocation)
      , info(other.info) {
    other.buffer = VK_NULL_HANDLE;
    other.allocation = nullptr;
    other.allocator = nullptr;
  }

  Buffer &operator=(Buffer &&other) noexcept {
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

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  void Cleanup() {
    if (buffer != VK_NULL_HANDLE && allocation != nullptr && allocator != nullptr)
      vmaDestroyBuffer(allocator, buffer, allocation);
    buffer = VK_NULL_HANDLE;
  }

  ~Buffer() {
    Cleanup();
  }

  void MapMemoryFromBytes(const void *src, size_t sizeBytes, size_t dstOffset = 0) {
    void *dst = nullptr;
    vmaMapMemory(allocator, allocation, &dst);
    std::memcpy(static_cast<char *>(dst) + dstOffset, src, sizeBytes);
    vmaFlushAllocation(allocator, allocation, dstOffset, sizeBytes);
    vmaUnmapMemory(allocator, allocation);
  }

  template <typename T>
  void MapMemoryFromScalar(const T &value, size_t dstOffset = 0) {
    MapMemoryFromBytes(&value, sizeof(T), dstOffset);
  }

  template <typename T>
  void MapMemoryFromVector(const std::vector<T> &vec, size_t dstOffset = 0) {
    if (vec.empty())
      return;
    MapMemoryFromBytes(vec.data(), vec.size() * sizeof(T), dstOffset);
  }

  template <typename T>
  void MapMemoryToScalar(T &value) {
    T *data;
    vmaMapMemory(allocator, allocation, reinterpret_cast<void **>(&data));
    value = *data;
    vmaUnmapMemory(allocator, allocation);
  }

  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocator allocator;
  VmaAllocation allocation;
  VmaAllocationInfo info{};
};
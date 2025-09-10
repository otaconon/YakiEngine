#include "Assets/Texture.h"

#include <stb_image.h>
#include <bit>

#include "Vulkan/VkUtils.h"

Texture::Texture(VulkanContext* ctx, fastgltf::Asset &gltfAsset, fastgltf::Image &gltfImage)
  : m_ctx{ctx} {
  int width, height, nrChannels;

  std::visit(
      fastgltf::visitor{
          [](auto &arg) {
          },
          [&](fastgltf::sources::URI &filePath) {
            assert(filePath.fileByteOffset == 0);
            assert(filePath.uri.isLocalPath());

            const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
            unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
            if (data) {
              VkExtent3D imagesize{
                  .width = static_cast<uint32_t>(width),
                  .height = static_cast<uint32_t>(height),
                  .depth = 1
              };

              createTexture(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

              stbi_image_free(data);
            }
          },
          [&](fastgltf::sources::Vector &vector) {
            unsigned char *data = stbi_load_from_memory(bit_cast<stbi_uc *>(vector.bytes.data()),
                static_cast<int>(vector.bytes.size()), &width, &height,
                &nrChannels, 4);
            if (data) {
              VkExtent3D imagesize{
                  .width = static_cast<uint32_t>(width),
                  .height = static_cast<uint32_t>(height),
                  .depth = 1
              };

              createTexture(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

              stbi_image_free(data);
            }
          },
          [&](fastgltf::sources::BufferView &view) {
            auto &bufferView = gltfAsset.bufferViews[view.bufferViewIndex];
            auto &buffer = gltfAsset.buffers[bufferView.bufferIndex];

            std::visit(
                fastgltf::visitor{
                    [](auto &arg) {
                      std::println(std::cerr, "Unhandled buffer data type");
                    },
                    [&](fastgltf::sources::Array &vector) {
                      stbi_uc *data = stbi_load_from_memory(std::bit_cast<const stbi_uc *>(vector.bytes.data() + bufferView.byteOffset), static_cast<int>(bufferView.byteLength), &width, &height, &nrChannels, 4);
                      if (data) {
                        VkExtent3D imagesize{
                            .width = static_cast<uint32_t>(width),
                            .height = static_cast<uint32_t>(height),
                            .depth = 1
                        };

                        createTexture(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

                        stbi_image_free(data);
                      }
                    }
                },
                buffer.data);
          },
      },
      gltfImage.data);
}

Texture::Texture(VulkanContext *ctx, void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
  : m_ctx{ctx} {
  createTexture(data, size, format, usage, mipmapped);
}

Texture::Texture(VulkanContext *ctx, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
  : m_ctx{ctx} {
  createTexture(size, format, usage, mipmapped);
}

Texture::~Texture() {
  Cleanup();
}

Texture::Texture(Texture &&other) noexcept
  : m_ctx{std::move(other.m_ctx)},
    m_allocator{other.m_allocator},
    m_allocation{other.m_allocation},
    m_image{other.m_image},
    m_view{other.m_view},
    m_extent{other.m_extent},
    m_format{other.m_format} {
  other.m_allocator = nullptr;
  other.m_allocation = nullptr;
  other.m_image = VK_NULL_HANDLE;
  other.m_view = VK_NULL_HANDLE;
}

Texture &Texture::operator=(Texture &&other) noexcept {
  if (this != &other) {
    Cleanup();

    m_ctx = std::move(other.m_ctx);
    m_allocator = other.m_allocator;
    m_allocation = other.m_allocation;
    m_image = other.m_image;
    m_view = other.m_view;
    m_extent = other.m_extent;
    m_format = other.m_format;

    other.m_allocator = nullptr;
    other.m_allocation = nullptr;
    other.m_image = VK_NULL_HANDLE;
    other.m_view = VK_NULL_HANDLE;
  }
  return *this;
}

VkImage Texture::GetImage() const { return m_image; }
VkImageView Texture::GetView() const { return m_view; }
VkExtent3D Texture::GetExtent() const { return m_extent; }
VkFormat Texture::GetFormat() const { return m_format; }

void Texture::createTexture(void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
  m_allocator = m_ctx->GetAllocator();
  m_format = format;
  m_extent = size;

  size_t data_size = size.depth * size.width * size.height * 4;
  Buffer uploadBuffer(m_allocator, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  memcpy(uploadBuffer.info.pMappedData, data, data_size);
  createImage(usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  m_ctx->ImmediateSubmit([&](VkCommandBuffer cmd) {
    VkUtil::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copyRegion{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageExtent = size
    };

    vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    if (mipmapped)
      generateMipMaps(cmd);
    else
      VkUtil::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  });
}

void Texture::createTexture(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
  m_allocator = m_ctx->GetAllocator();
  m_format = format;
  m_extent = size;

  createImage(usage);

  m_ctx->ImmediateSubmit([&](VkCommandBuffer cmd) {
    VkImageLayout finalLayout = getFinalLayout(format, usage);
    VkUtil::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_UNDEFINED, finalLayout);
  });
}

void Texture::createImage(VkImageUsageFlags usage) {
  m_mipLevels = static_cast<int>(std::floor(std::log2(std::max(m_extent.width, m_extent.height)))) + 1;
  VkImageCreateInfo imgInfo = VkInit::image_create_info(m_format, usage, m_extent, m_mipLevels);

  VmaAllocationCreateInfo allocInfo{
      .usage = VMA_MEMORY_USAGE_GPU_ONLY,
      .requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
  };

  VK_CHECK(vmaCreateImage(m_allocator, &imgInfo, &allocInfo, &m_image, &m_allocation, nullptr));

  VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
  if (m_format == VK_FORMAT_D32_SFLOAT)
    aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;

  VkImageViewCreateInfo view_info = VkInit::imageview_create_info(m_format, m_image, aspectFlag);
  view_info.subresourceRange.levelCount = imgInfo.mipLevels;

  VK_CHECK(vkCreateImageView(m_ctx->GetDevice(), &view_info, nullptr, &m_view));
}

void Texture::generateMipMaps(VkCommandBuffer cmd) {
  for (int mip = 0; mip < m_mipLevels; mip++) {
    VkExtent2D halfSize = VkExtent2D{m_extent.width, m_extent.height};
    halfSize.width /= 2;
    halfSize.height /= 2;

    VkImageMemoryBarrier2 imageBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr};
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange = VkInit::image_subresource_range(aspectMask);
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseMipLevel = mip;
    imageBarrier.image = m_image;

    VkDependencyInfo depInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .pNext = nullptr};
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);

    if (mip < m_mipLevels - 1) {
      VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};
      blitRegion.srcOffsets[1].x = m_extent.width;
      blitRegion.srcOffsets[1].y = m_extent.height;
      blitRegion.srcOffsets[1].z = 1;
      blitRegion.dstOffsets[1].x = halfSize.width;
      blitRegion.dstOffsets[1].y = halfSize.height;
      blitRegion.dstOffsets[1].z = 1;
      blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blitRegion.srcSubresource.baseArrayLayer = 0;
      blitRegion.srcSubresource.layerCount = 1;
      blitRegion.srcSubresource.mipLevel = mip;
      blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blitRegion.dstSubresource.baseArrayLayer = 0;
      blitRegion.dstSubresource.layerCount = 1;
      blitRegion.dstSubresource.mipLevel = mip + 1;

      VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
      blitInfo.dstImage = m_image;
      blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      blitInfo.srcImage = m_image;
      blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      blitInfo.filter = VK_FILTER_LINEAR;
      blitInfo.regionCount = 1;
      blitInfo.pRegions = &blitRegion;

      vkCmdBlitImage2(cmd, &blitInfo);

      m_extent.width = halfSize.width;
      m_extent.height = halfSize.height;
    }
  }

  VkUtil::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Texture::Cleanup() {
  if (m_image != VK_NULL_HANDLE && m_view != VK_NULL_HANDLE) {
    vkDestroyImageView(m_ctx->GetDevice(), m_view, nullptr);
    vmaDestroyImage(m_allocator, m_image, m_allocation);
  }
  m_view = VK_NULL_HANDLE;
  m_image = VK_NULL_HANDLE;
}

VkImageLayout Texture::getFinalLayout(VkFormat format, VkImageUsageFlags usage) {
  if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM)
    return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

  if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
    return VK_IMAGE_LAYOUT_GENERAL;

  if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  return VK_IMAGE_LAYOUT_GENERAL;
}
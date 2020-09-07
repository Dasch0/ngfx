#include "ngfx.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace ngfx 
{
  gBuffer::gBuffer(Context *context)
    : c(context), valid(false) 
  {}

  void gBuffer::init(vk::DeviceSize size, vk::BufferUsageFlags usage)
  {
    VkBufferCreateInfo stagingInfo =  vk::BufferCreateInfo(
        vk::BufferCreateFlagBits(),
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        // TODO: support concurrent
        vk::SharingMode::eExclusive,
        0,
        nullptr);

    VmaAllocationCreateInfo vmaStagingInfo = {};
    vmaStagingInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    vmaCreateBuffer(
        c->allocator,
        &stagingInfo,
        &vmaStagingInfo,
        &stagingBuffer,
        &stagingMemory,
        nullptr);

    VkBufferCreateInfo localInfo =  vk::BufferCreateInfo(
        vk::BufferCreateFlagBits(),
        size,
        vk::BufferUsageFlagBits::eTransferDst
        | usage,
        // TODO: support concurrent
        vk::SharingMode::eExclusive,
        0,
        nullptr);

    VmaAllocationCreateInfo vmaLocalInfo = {};
    vmaLocalInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateBuffer(
        c->allocator,
        &localInfo,
        &vmaLocalInfo,
        &localBuffer,
        &localMemory,
        nullptr);

    // Create and record transfer command buffer
    vk::CommandBufferAllocateInfo allocInfo(c->transferPool,
                                            vk::CommandBufferLevel::ePrimary,
                                            1);
    c->device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(),
                                         nullptr);
    commandBuffer.begin(beginInfo);
    vk::BufferCopy copyRegion(0, 0, size);
    commandBuffer.copyBuffer(stagingBuffer,
                             localBuffer,
                             1,
                             (const vk::BufferCopy *) &copyRegion);
    commandBuffer.end();

    // Set valid state
    valid = true;
  }

  void *gBuffer::map(void)
  {
    void *data;
    assert(valid);
    vmaMapMemory(c->allocator, stagingMemory, &data);
    return data;
  }

  void gBuffer::unmap(void)
  {
    vmaUnmapMemory(c->allocator, stagingMemory);
  }

  // TODO: add fences for external sync
  void gBuffer::copy(void)
  {
    assert(valid);
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);
    c->transferQueue.submit(1, &submitInfo, vk::Fence());
  }

  void gBuffer::blockingCopy()
  {
    assert(valid);
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);
    c->transferQueue.submit(1, &submitInfo, vk::Fence());
    c->transferQueue.waitIdle();
    c->graphicsQueue.waitIdle();
  }

  gBuffer::~gBuffer(void)
  {
    unmap();
    if (valid)
    {
      c->device.freeCommandBuffers(c->transferPool,
                                 1,
                                 (const vk::CommandBuffer *)&commandBuffer);
      vmaDestroyBuffer(c->allocator, stagingBuffer, stagingMemory);
      vmaDestroyBuffer(c->allocator, localBuffer, localMemory);
    }
  }
}

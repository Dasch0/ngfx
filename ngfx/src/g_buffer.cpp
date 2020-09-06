#include "ngfx.hpp"
#include <vulkan/vulkan.hpp>

namespace ngfx 
{
  gBuffer::gBuffer(Context *context)
    : c(context), valid(false) 
  {}

  void gBuffer::init(vk::DeviceSize size, vk::BufferUsageFlags usage)
  {
    vk::BufferCreateInfo stagingCI(vk::BufferCreateFlagBits(),
                                  size,
                                  vk::BufferUsageFlagBits::eTransferSrc,
                                  // TODO: support concurrent
                                  vk::SharingMode::eExclusive,
                                  0,
                                  nullptr);
    c->device.createBuffer(&stagingCI,
                         nullptr,
                         &stagingBuffer);

    vk::BufferCreateInfo localCI(vk::BufferCreateFlagBits(),
                                  size,
                                  vk::BufferUsageFlagBits::eTransferDst
                                  | usage,
                                  // TODO: support concurrent
                                  vk::SharingMode::eExclusive,
                                  0,
                                  nullptr);
    c->device->createBuffer(&localCI,
                         nullptr,
                         &localBuffer);

    vk::MemoryRequirements stagingReqs =
        c->device.getBufferMemoryRequirements(stagingBuffer);
    vk::MemoryRequirements localReqs =
        c->device.getBufferMemoryRequirements(localBuffer);

    uint32_t stagingIndex =
        util::findMemoryType(c->physicalDevice,
                             stagingReqs.memoryTypeBits,
                             vk::MemoryPropertyFlagBits::eHostVisible
                             | vk::MemoryPropertyFlagBits::eHostCoherent
                             | vk::MemoryPropertyFlagBits::eHostCached);
    uint32_t localIndex =
        util::findMemoryType(c->physicalDevice,
                             localReqs.memoryTypeBits,
                             vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::MemoryAllocateInfo stagingAllocInfo(stagingReqs.size, stagingIndex);
    c->device.allocateMemory(&stagingAllocInfo, nullptr, &stagingMemory);
    c->device.bindBufferMemory(stagingBuffer, stagingMemory, 0);
    vk::MemoryAllocateInfo localAllocInfo(localReqs.size, localIndex);
    c->device.allocateMemory(&stagingAllocInfo, nullptr, &localMemory);
    c->device.bindBufferMemory(localBuffer, localMemory, 0);

    // Create and record transfer command buffer
    vk::CommandBufferAllocateInfo allocInfo(c->cmdPool,
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

    // Map memory
    device->mapMemory(stagingMemory, 0, size, vk::MemoryMapFlags(), &_handle);
    valid = true;
  }

  void gBuffer::map(void* data)
  {
    assert(valid);
    memcpy(_handle, data, size);
  }

  // TODO: add fences for external sync
  void gBuffer::copy(vk::Queue q)
  {
    assert(valid);
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);
    q.submit(1, &submitInfo, vk::Fence());
  }

  void gBuffer::blockingCopy(vk::Queue q)
  {
    assert(valid);
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);
    q.submit(1, &submitInfo, vk::Fence());
    q.waitIdle();
  }

  gBuffer::~gBuffer(void)
  {
    if (valid)
    {
      device->unmapMemory(stagingMemory);
      device->freeCommandBuffers(*pool,
                                 1,
                                 (const vk::CommandBuffer *)&commandBuffer);
      device->freeMemory(stagingMemory);
      device->freeMemory(localMemory);
      device->destroyBuffer(stagingBuffer);
      device->destroyBuffer(localBuffer);
    }
  }

}

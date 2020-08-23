#include "fast_buffer.hpp"
#include <vulkan/vulkan.hpp>
#include "context.hpp"

namespace ngfx 
{  
  template<typename T, size_t N>
  FastBuffer<T, N>::FastBuffer(
      Context *context,
      vk::BufferUsageFlags usage,
      T initData[N])
    : c(context), usage(usage), count(N), size(N * sizeof(T)), data(initData)
  {

    // Create Buffers
    vk::BufferCreateInfo stagingCI(
        vk::BufferCreateFlagBits(),
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        // TODO: support concurrent
        vk::SharingMode::eExclusive,
        0,
        nullptr);
    
    c->device.createBuffer(
        &stagingCI,
        nullptr,
        &stagingBuffer);

    vk::BufferCreateInfo localCI(
        vk::BufferCreateFlagBits(),
        size,
        vk::BufferUsageFlagBits::eTransferDst
        | usage,
        // TODO: support concurrent
        vk::SharingMode::eExclusive,
        0,
        nullptr);
    
    c->device.createBuffer(
         &localCI,
         nullptr,
         &localBuffer);

    vk::MemoryRequirements stagingReqs =
        c->device.getBufferMemoryRequirements(stagingBuffer);
    vk::MemoryRequirements localReqs =
        c->device.getBufferMemoryRequirements(localBuffer);

    uint32_t stagingIndex = util::findMemoryType(
        c->physicalDevice,
        stagingReqs.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible
        | vk::MemoryPropertyFlagBits::eHostCoherent
        | vk::MemoryPropertyFlagBits::eHostCached);
    
    uint32_t localIndex = util::findMemoryType(
        c->physicalDevice,
        localReqs.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Allocate Buffers
    vk::MemoryAllocateInfo stagingAllocInfo(stagingReqs.size, stagingIndex);
    c->device.allocateMemory(&stagingAllocInfo, nullptr, &stagingMemory);
    c->device.bindBufferMemory(stagingBuffer, stagingMemory, 0);
    vk::MemoryAllocateInfo localAllocInfo(localReqs.size, localIndex);
    c->device.allocateMemory(&stagingAllocInfo, nullptr, &localMemory);
    c->device.bindBufferMemory(localBuffer, localMemory, 0);

    // Create and record transfer command buffer
    vk::CommandBufferAllocateInfo allocInfo(
        c->cmdPool,
        vk::CommandBufferLevel::ePrimary,
        1);
    c->device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo(
        vk::CommandBufferUsageFlags(),
        nullptr);
      
    commandBuffer.begin(beginInfo);
    vk::BufferCopy copyRegion(0, 0, size);
    
    commandBuffer.copyBuffer(
        stagingBuffer,
        localBuffer,
        1,
        (const vk::BufferCopy *) &copyRegion);
      
    commandBuffer.end();
    
    // Map memory
    c->device.mapMemory(
        stagingMemory,
        0,
        size,
        vk::MemoryMapFlags(),
        data);
  }

  // TODO: add fences for external sync
  template<typename T, size_t N>
  void FastBuffer<T, N>::copy(void)
  {
    vk::SubmitInfo submitInfo(
        0,
        nullptr,
        nullptr,
        1,
        &commandBuffer,
        0,
        nullptr);
    
    c->transferQueue.submit(1, &submitInfo, vk::Fence());
  }

  template<typename T, size_t N>
  void FastBuffer<T, N>::blockingCopy(void)
  {
    vk::SubmitInfo submitInfo(
        0,
        nullptr,
        nullptr,
        1,
        &commandBuffer,
        0,
        nullptr);
    
    c->graphicsQueue.submit(1, &submitInfo, vk::Fence());
    c->graphicsQueue.waitIdle();
  }

  template<typename T, size_t N>
  FastBuffer<T, N>::~FastBuffer(void)
  {
    c->device.unmapMemory(stagingMemory);
    c->device.freeCommandBuffers(
        c->cmdPool,
        1,
        (const vk::CommandBuffer *)&commandBuffer);
    
    c->device.freeMemory(stagingMemory);
    c->device.freeMemory(localMemory);
    c->device.destroyBuffer(stagingBuffer);
    c->device.destroyBuffer(localBuffer);
  }
}

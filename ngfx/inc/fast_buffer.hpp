#ifndef NGFX_FAST_BUFFER
#define NGFX_FAST_BUFFER

#include <vulkan/vulkan.hpp>
#include "util.hpp"
#include "context.hpp"

namespace ngfx
{
  // Abstracts buffer and transfer semantics for a fast uniform/vertex buffer
  // that is easy to work with on the CPU side
  // TODO: batch buffer allocations
  template <typename T, size_t N>
  class FastBuffer
  {
  public:
    Context *c;
    vk::BufferUsageFlags usage;
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingMemory;
    vk::Buffer localBuffer;
    vk::DeviceMemory localMemory;
    vk::CommandBuffer commandBuffer;
    size_t count;
    size_t size;
    T data[N];

    FastBuffer(Context *context, vk::BufferUsageFlags usage, T initData[N]);

    void copy(void);
    void blockingCopy(void);
    ~FastBuffer(void);
  };
}

#endif // NGFX_FAST_BUFFER

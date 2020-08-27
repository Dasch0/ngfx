#ifndef NGFX_MESH_TABLE
#define NGFX_MESH_TABLE

#include <vulkan/vulkan.hpp>
#include "util.hpp"
#include "fast_buffer.hpp"
#include "context.hpp"

namespace ngfx
{
  template<size_t CNT, size_t SZ>
  struct MeshTable
  {
    FastBuffer<util::Vertex, SZ> mesh;
    FastBuffer<uint16_t, SZ> indices;
    FastBuffer<util::Instance, CNT> instances;

    MeshTable(Context *c) 
      : mesh(c, vk::BufferUsageFlagBits::eVertexBuffer, {0}),
        indices(c, vk::BufferUsageFlagBits::eIndexBuffer, {0}),
        instances(c, vk::BufferUsageFlagBits::eVertexBuffer, {0})
    {}
  };
}


#endif // NGFX_MESH_TABLE

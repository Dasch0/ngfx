#ifndef NGFX_MESH_TABLE
#define NGFX_MESH_TABLE

#include "util.hpp"
#include "fast_buffer.hpp"
#include <bits/stdint-uintn.h>
 
namespace ngfx
{
  template<size_t CNT, size_t SZ>
  struct MeshTable
  {
    FastBuffer<util::Vertex, SZ> mesh;
    FastBuffer<uint16_t, SZ> indices;
    FastBuffer<util::Instance, CNT> instances;
    
  };
}


#endif // NGFX_MESH_TABLE

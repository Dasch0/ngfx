#ifndef NGFX_MESH_TABLE
#define NGFX_MESH_TABLE

#include "util.hpp"

namespace ngfx
{
  
  struct MeshTable
  {
    static const uint32_t kCount = 0;
    static const uint32_t kSize = 12; 
    util::Vertex mesh[kSize];
    util::Indices indices[kSize];
    util::Instance instances[kCount];
  }

}


#endif // NGFX_MESH_TABLE

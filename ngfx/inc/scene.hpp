#ifndef NGFX_SCENE_H
#define NGFX_SCENE_H

#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"

namespace ngfx
{
  struct Scene
  {
    vk::RenderPass pass;
    std::vector<vk::Framebuffer> frames;

    // Pointer for device held for use in destructor only
    // Pointer must stay valid for lifetime
    vk::Device *device;

    Scene(Context *c, SwapData *s);
    ~Scene();
  };
}

#endif //NGFX_SCENE_H

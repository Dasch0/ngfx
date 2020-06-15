#ifndef NGFX_OVERLAY_H
#define NGFX_OVERLAY_H

#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"

namespace ngfx
{
  struct Overlay
  {
    vk::RenderPass pass;
    std::vector<vk::Framebuffer> frames;
    vk::Sampler sampler;
    // Pointer for device held for use in destructor only
    // Pointer must stay valid for lifetime
    vk::Device *device;

    Overlay(Context *c, SwapData *s);
    ~Overlay();
  };
}

#endif //NGFX_OVERLAY_H

#ifndef NGFX_OVERLAY_H
#define NGFX_OVERLAY_H

#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"

namespace ngfx
{
  struct Overlay
  {
    static vk::VertexInputAttributeDescription attribute[];
    static vk::VertexInputBindingDescription binding[]; 

    vk::RenderPass pass;
    std::vector<vk::Framebuffer> frames;
    vk::Sampler sampler;
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;
    vk::DescriptorSetLayout descLayout;
    // Pointer for device held for use in destructor only
    // Pointer must stay valid for lifetime
    vk::Device *device;
    
    Overlay(Context *c, SwapData *s);
    ~Overlay();
  };
}

#endif //NGFX_OVERLAY_H

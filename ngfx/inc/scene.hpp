#ifndef NGFX_SCENE_H
#define NGFX_SCENE_H

#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"
#include "util.hpp"

namespace ngfx
{
  struct Scene
  {
    static vk::VertexInputAttributeDescription attribute[];
    static vk::VertexInputBindingDescription binding[];
    vk::RenderPass pass;
    std::vector<vk::Framebuffer> frames;
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;
    util::Mvp mvp;

    // Pointer for device held for use in destructor only
    // Pointer must stay valid for lifetime
    vk::Device *device;

    Scene(Context *c, SwapData *s);
    ~Scene();
  };
}

#endif //NGFX_SCENE_H

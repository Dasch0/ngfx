#ifndef NGFX_SCENE_H
#define NGFX_SCENE_H

#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"
#include "util.hpp"
#include "camera.hpp"

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
    
    Camera cam;
    util::FastBuffer camBuffer;

    vk::DescriptorSetLayout descLayout;
    vk::DescriptorPool descPool;
    vk::DescriptorSet descSet;

    // Pointer for device held for use in destructor only
    // Pointer must stay valid for lifetime
    vk::Device *device;
    vk::Queue *q;
    Scene(Context *c, SwapData *s);
    ~Scene();

    private:
      void createDescriptorPool(void);
      void createDescriptorSets(void);
  };
}

#endif //NGFX_SCENE_H

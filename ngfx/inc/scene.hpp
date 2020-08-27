#ifndef NGFX_SCENE_H
#define NGFX_SCENE_H

#include "vulkan/vulkan.hpp"
#include "config.hpp"
#include "swap_data.hpp"
#include "util.hpp"
#include "camera.hpp"
#include "fast_buffer.hpp"
#include "context.hpp"


namespace ngfx
{
  struct Scene
  {
    static vk::VertexInputAttributeDescription attribute[];
    static vk::VertexInputBindingDescription binding[];
    vk::RenderPass pass;

    vk::PipelineLayout layout;
    vk::Pipeline pipeline;
    Camera<1> cam;
    vk::DescriptorSetLayout descLayout;
    vk::DescriptorPool descPool;
    vk::DescriptorSet descSet;

    // Pointer for device held for use in destructor only
    // Pointer must stay valid for lifetime
    Context *c;
    SwapData *s;

    uint32_t cmdCount;
    
    vk::Framebuffer frames[kMaxSwapImages];
    vk::CommandBuffer cmdBuff[kMaxSwapImages];

    Scene(Context *pContext, SwapData *pSwap);
    ~Scene();

    private:
      void createDescriptorPool(void);
      void createDescriptorSets(void);
      void buildCommandBuffers(void);
  };
}

#endif //NGFX_SCENE_H

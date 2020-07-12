#ifndef NGFX_OVERLAY_H
#define NGFX_OVERLAY_H

#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"

namespace ngfx
{
  class Overlay
  {
    public:
      static vk::VertexInputAttributeDescription attribute[];
      static vk::VertexInputBindingDescription binding[]; 

      vk::RenderPass pass;
      std::vector<vk::Framebuffer> frames;
      vk::Sampler sampler;
      vk::PipelineLayout layout;
      vk::Pipeline pipeline;
      vk::DescriptorSetLayout descLayout;
      vk::DescriptorPool descPool;
      vk::DescriptorSet descSet;

      // current image view used by overlay
      vk::ImageView view;

      // Pointer for device held for use in destructor only
      // Pointer must stay valid for lifetime
      vk::Device *device;

      Overlay(Context *c, SwapData *s, vk::ImageView v);

      void updateSampler(vk::ImageView view);

      ~Overlay();

    private:
      void createDescriptorPool(void);
      void createDescriptorSets(void);
  };
}

#endif //NGFX_OVERLAY_H

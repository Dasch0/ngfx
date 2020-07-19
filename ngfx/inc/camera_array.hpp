#ifndef NGFX_CAMERAARRAY_H
#define NGFX_CAMERAARRAY_H

#include "vulkan/vulkan.hpp"
#include "util.hpp"
#include "context.hpp"
#include "camera.hpp"

namespace ngfx
{
  struct CameraArray
  {
    public:
      uint w = 256;
      uint h = 256;
      static vk::VertexInputAttributeDescription attribute[];
      static vk::VertexInputBindingDescription binding[];
      vk::RenderPass pass;
      util::Fbo fbo;
      vk::PipelineLayout layout;
      vk::Pipeline pipeline;
      
      Camera cam;
      util::FastBuffer camBuffer;

      vk::DescriptorSetLayout descLayout;
      vk::DescriptorPool descPool;
      vk::DescriptorSet descSet;

      // Pointer to device, used for destructor
      vk::Device *device;
      // TODO:: Remove this once testing is completed
      vk::Queue *q;
      CameraArray(Context *c);
      ~CameraArray(void);
    
    private:
      void buildFbo(Context *c);
      void buildRenderPass(void);
      void createDescriptorPool(void);
      void createDescriptorSets(void);
  };
}

#endif //NGFX_CAMERAARRAY_H

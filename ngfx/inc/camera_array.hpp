#ifndef NGFX_CAMERAARRAY_H
#define NGFX_CAMERAARRAY_H

#include "vulkan/vulkan.hpp"
#include "util.hpp"
#include "context.hpp"

namespace ngfx
{
  struct CameraArray
  {
    public:
    static vk::VertexInputAttributeDescription attribute[];
    static vk::VertexInputBindingDescription binding[];
    vk::RenderPass pass;
    util::Fbo fbo;
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;
    util::Mvp mvp;
    
    // Pointer to device, used for destructor
    vk::Device *device;

    CameraArray(Context *c);

    ~CameraArray(void);
  
    private:
    void buildFbo(Context *c);
    void buildRenderPass(void);
  };
}

#endif //NGFX_CAMERAARRAY_H

#ifndef NGFX_SWAPDATA_H
#define NGFX_SWAPDATA_H

#include <vector>
#include "config.hpp"
#include "vulkan/vulkan.hpp"
#include "context.hpp"

// TODO: Docs
namespace ngfx
{
  // TODO: Maybe try to find a way to avoid vector use here
  // Could possibly do something like fixed array with max images supported
  // TODO: Support Swapchain rebuild for resizability
 struct SwapData {
    vk::SwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
    
    //reference to Context, only used for destructor
    Context *c;

    uint32_t swapCount;
    vk::Image images[kMaxSwapImages];
    vk::ImageView views[kMaxSwapImages];
    vk::Fence fences[kMaxSwapImages];
    
    
    SwapData(Context *pContext);
    ~SwapData();
  };
}

#endif //NGFX_SWAPDATA_H

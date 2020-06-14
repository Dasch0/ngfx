#ifndef NGFX_SWAPDATA_H
#define NGFX_SWAPDATA_H

#include <vector>
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
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> views;
    std::vector<vk::Fence> fences;
    vk::Format format;
    vk::Extent2D extent;
    
    //reference to device, only used for destructor
    vk::Device *device;
    
    SwapData(Context *pContext);
    ~SwapData();
  };
}

#endif //NGFX_SWAPDATA_H

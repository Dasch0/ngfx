#include "swap_data.hpp"
#include "config.hpp"

namespace ngfx
{
  SwapData::SwapData(Context *pContext) 
    : c(pContext)
  {
    
    // Swapchain & Format
    vk::SurfaceFormatKHR surfaceForm =
        util::chooseSwapSurfaceFormat(c->swapInfo.formats);
    vk::PresentModeKHR presentMode =
        util::chooseSwapPresentMode(c->swapInfo.presentModes);

    vk::Extent2D swapExtent(c->kWidth, c->kHeight);

    format = surfaceForm.format;
    extent = swapExtent;

    uint32_t imageCount = c->swapInfo.capabilites.minImageCount + 1;
    if (c->swapInfo.capabilites.maxImageCount > 0 &&
        imageCount > c->swapInfo.capabilites.maxImageCount) {
      imageCount = c->swapInfo.capabilites.maxImageCount;
    }
    
    uint32_t indices[] = {
      c->qFamilies.graphicsFamily.value(),
      c->qFamilies.presentFamily.value()
    };
    bool unifiedQ = (c->qFamilies.graphicsFamily.value()
                         == c->qFamilies.presentFamily.value());

    vk::SwapchainCreateInfoKHR
      swapchainCI(vk::SwapchainCreateFlagsKHR(),
                  c->surface,
                  imageCount,
                  format,
                  surfaceForm.colorSpace,
                  extent,
                  1,
                  vk::ImageUsageFlagBits::eColorAttachment,
                  (unifiedQ) ? vk::SharingMode::eExclusive
                             : vk::SharingMode::eConcurrent,
                  (unifiedQ) ? 0
                             : 2,
                  (unifiedQ) ? nullptr
                             : indices,
                  c->swapInfo.capabilites.currentTransform,
                  vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode,
                  VK_TRUE,             // clipped
                  vk::SwapchainKHR()); // old swapchain 
    c->device.createSwapchainKHR(&swapchainCI, nullptr, &swapchain);

    // Images & Views
    c->device.getSwapchainImagesKHR(swapchain,
                                 &swapCount,
                                 (vk::Image *) nullptr);

    assert(swapCount > kMaxSwapImages);
    
    c->device.getSwapchainImagesKHR(swapchain,
                                 &swapCount,
                                 images);
    
    for (size_t i = 0; i < swapCount; i++)
    {
      vk::ImageViewCreateInfo
        viewCI(vk::ImageViewCreateFlags(),
               images[i],
               vk::ImageViewType::e2D,
               format,
               vk::ComponentMapping(),
               vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,
                                         0, // baseMipLevel
                                         1, // levelCount
                                         0, // baseArrayLayer
                                         1) // layerCount
      );
      c->device.createImageView(&viewCI, nullptr, &views[i]);
    }  
  }

  SwapData::~SwapData()
  {
    for (auto view : views)
    {
      c->device.destroyImageView(view);
    }
    c->device.destroySwapchainKHR(swapchain);
  }
}

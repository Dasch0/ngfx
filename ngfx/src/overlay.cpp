#include "overlay.hpp"
#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"

namespace ngfx
{
  Overlay::Overlay(Context *c, SwapData *s)
    : device(&c->device)
  {
    // RenderPass
    vk::AttachmentDescription
      colorAttachment(vk::AttachmentDescriptionFlags(),
                      s->format, 
                      vk::SampleCountFlagBits::e1,
                      vk::AttachmentLoadOp::eDontCare,
                      vk::AttachmentStoreOp::eStore,
                      vk::AttachmentLoadOp::eDontCare,
                      vk::AttachmentStoreOp::eDontCare,
                      vk::ImageLayout::eUndefined,
                      vk::ImageLayout::ePresentSrcKHR);
    vk::AttachmentReference colorAttachmentRef(
        0,
        vk::ImageLayout::eColorAttachmentOptimal);
    
    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &colorAttachmentRef,
        nullptr,
        nullptr,
        0,
        nullptr);
    
    vk::SubpassDependency subpassDependency(
        0,
        VK_SUBPASS_EXTERNAL,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlags());
    
    vk::RenderPassCreateInfo renderPassCI(
        vk::RenderPassCreateFlags(),
        1,
        &colorAttachment,
        1,
        &subpass,
        1,
        &subpassDependency);
    
    c->device.createRenderPass(&renderPassCI, nullptr, &pass);
    
    // Framebuffers
    frames.resize(s->views.size());
    for(size_t i = 0; i < s->views.size(); i++)
    {
      vk::ImageView attachments[] = {
        s->views[i]
      };

      vk::FramebufferCreateInfo framebufferCI(
          vk::FramebufferCreateFlags(),
          pass,
          1,
          attachments,
          s->extent.width,
          s->extent.height,
          1);
      
      c->device.createFramebuffer(
          &framebufferCI,
          nullptr,
          &frames[i]);
    }

    // Image Sampler
    vk::SamplerCreateInfo samplerCI(
        vk::SamplerCreateFlags(),
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge,
        0.0f,
        false,
        1.0f, // TODO: support anisotropy levels
        false,
        vk::CompareOp::eAlways,
        0.0f,
        0.0f,
        vk::BorderColor::eIntTransparentBlack,
        false);
    
    c->device.createSampler
      (&samplerCI,
       nullptr,
       &sampler);
  }

  Overlay::~Overlay()
  {
    for(vk::Framebuffer &frame : frames)
    {
      device->destroyFramebuffer(frame);
    }

    device->destroyRenderPass(pass);
  }
}
#include "pipeline.hpp"
#include "overlay.hpp"
#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"
#include "util.hpp"

namespace ngfx
{
  vk::VertexInputAttributeDescription Overlay::attribute[] = {
    // Per Vertex data
    vk::VertexInputAttributeDescription(
        0,
        0,
        vk::Format::eR32G32Sfloat,
        offsetof(util::Vertex, pos)),
    vk::VertexInputAttributeDescription(
        1,
        0,
        vk::Format::eR32G32B32Sfloat,
        offsetof(util::Vertex, color)),
    vk::VertexInputAttributeDescription(
        2,
        0,
        vk::Format::eR32G32Sfloat,
        offsetof(util::Vertex, texCoord)),
  };

  vk::VertexInputBindingDescription Overlay::binding[] = {
    vk::VertexInputBindingDescription(
        0,
        sizeof(util::Vertex),
        vk::VertexInputRate::eVertex),
  };

  void Overlay::createDescriptorPool(void)
  {
    vk::DescriptorPoolSize poolSize[] = {
      vk::DescriptorPoolSize(
          vk::DescriptorType::eCombinedImageSampler,
          1)
    };

    vk::DescriptorPoolCreateInfo poolInfo(
        vk::DescriptorPoolCreateFlags(),
        1,
        util::array_size(poolSize),
        poolSize); 
    
    device->createDescriptorPool(
       &poolInfo, 
       nullptr, 
       &descPool);
  }

  void Overlay::createDescriptorSets(void)
  {
    vk::DescriptorSetAllocateInfo allocInfo(descPool,
                                            1,
                                            &descLayout);

    device->allocateDescriptorSets(&allocInfo, &descSet);

    vk::DescriptorImageInfo imageInfo(
        sampler,
        view,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::WriteDescriptorSet descWrite[] = { 
      vk::WriteDescriptorSet(
          descSet,
          0,
          0,
          1,
          vk::DescriptorType::eCombinedImageSampler,
          &imageInfo,
          nullptr,
          nullptr)
    };

    device->updateDescriptorSets(
        util::array_size(descWrite),
        descWrite,
        0,
        nullptr);
  }

  Overlay::Overlay(Context *c, SwapData *s, vk::ImageView v)
    : device(&c->device), view(v)
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

    // Descriptors
    vk::DescriptorSetLayoutBinding bindings[] = {
     vk::DescriptorSetLayoutBinding(
         0,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eFragment, 
         nullptr)
    };
    
    vk::DescriptorSetLayoutCreateInfo layoutCI(
        vk::DescriptorSetLayoutCreateFlags(),
        util::array_size(bindings),
        bindings); 
    
    device->createDescriptorSetLayout
      (&layoutCI,
       nullptr,
       &descLayout);

    // Layout
    util::buildLayout(
        device,
        1,
        &descLayout,
        sizeof(glm::vec2),
        &layout);
    
    // Pipeline
    util::buildPipeline(
        device,
        s->extent,
        binding,
        util::array_size(binding),
        attribute,
        util::array_size(attribute),
        "shaders/overlay_vert.spv",
        "shaders/overlay_frag.spv",
        true,
        &layout,
        &pass,
        &c->pipelineCache,
        &pipeline);
    
    createDescriptorPool();
    createDescriptorSets();
  }

  Overlay::~Overlay()
  {
    device->destroyDescriptorPool(descPool);
    device->destroyDescriptorSetLayout(descLayout);

    for(vk::Framebuffer &frame : frames)
    {
      device->destroyFramebuffer(frame);
    }

    device->destroyRenderPass(pass);
  }
}

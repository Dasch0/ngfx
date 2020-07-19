#include "scene.hpp"
#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "swap_data.hpp"
#include "pipeline.hpp"

namespace ngfx
{
  // TODO: clean up vertex input attribute and binding creation
  // TODO: replace with lightweight vector class TBD
  vk::VertexInputAttributeDescription Scene::attribute[] = {
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
    // Per Instance Data
    vk::VertexInputAttributeDescription(
        3,
        1,
        vk::Format::eR32G32Sfloat,
        offsetof(util::Instance, pos)),
  };

  vk::VertexInputBindingDescription Scene::binding[] = {
    vk::VertexInputBindingDescription(
        0,
        sizeof(util::Vertex),
        vk::VertexInputRate::eVertex),
    vk::VertexInputBindingDescription(
        1,
        sizeof(util::Instance),
        vk::VertexInputRate::eInstance)
  };

  Scene::Scene(Context *c, SwapData *s)
    : device(&c->device), cam(s->extent), 
      camBuffer(
          &c->device,
          &c->physicalDevice,
          &c->cmdPool,
          sizeof(cam.cam),
          vk::BufferUsageFlagBits::eUniformBuffer)
  {
    // RenderPass
    vk::AttachmentDescription
      colorAttachment(vk::AttachmentDescriptionFlags(),
                      s->format, 
                      vk::SampleCountFlagBits::e1,
                      vk::AttachmentLoadOp::eClear,
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

    //Descriptors & buffers
    vk::DescriptorSetLayoutBinding bindings[] = {
      vk::DescriptorSetLayoutBinding(
         0,
         vk::DescriptorType::eUniformBuffer,
         1,
         vk::ShaderStageFlagBits::eVertex, 
         nullptr)
    };
    
    vk::DescriptorSetLayoutCreateInfo layoutCI(
        vk::DescriptorSetLayoutCreateFlags(),
        util::array_size(bindings),
        bindings); 
    
    device->createDescriptorSetLayout(
        &layoutCI,
        nullptr,
        &descLayout);

    vk::PipelineLayoutCreateInfo pipelineLayoutCI(
        vk::PipelineLayoutCreateFlags(),
        1,
        &descLayout,
        0,
        nullptr);

    device->createPipelineLayout(
        &pipelineLayoutCI,
        nullptr,
        &layout);

    util::buildPipeline(
        &c->device,
        s->extent,
        binding,
        util::array_size(binding),
        attribute,
        util::array_size(attribute),
        "shaders/env_vert.spv",
        "shaders/env_frag.spv",
        false,
        &layout,
        &pass,
        &c->pipelineCache,
        &pipeline);

    camBuffer.init();
    createDescriptorPool();
    createDescriptorSets();
    camBuffer.stage(&cam.cam);

    // TODO: Fix hack that stores queue here, prefer to restructure fastbuffer
    q = &c->graphicsQueue;
    camBuffer.blockingCopy(c->graphicsQueue);
  }

  void Scene::createDescriptorPool(void) {
    vk::DescriptorPoolSize poolSize[] = {
      vk::DescriptorPoolSize(
          vk::DescriptorType::eUniformBuffer,
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

  void Scene::createDescriptorSets(void)
  {
    vk::DescriptorSetAllocateInfo allocInfo(descPool,
                                            1,
                                            &descLayout);

    device->allocateDescriptorSets(&allocInfo, &descSet);

    vk::DescriptorBufferInfo buffInfo(
        camBuffer.localBuffer,
        0,
        sizeof(cam.cam));
    
    vk::WriteDescriptorSet descWrite[] = { 
      vk::WriteDescriptorSet(
          descSet,
          0,
          0,
          1,
          vk::DescriptorType::eUniformBuffer,
          nullptr,
          &buffInfo,
          nullptr)
    };

    device->updateDescriptorSets(
        util::array_size(descWrite),
        descWrite,
        0,
        nullptr); 
  }
 
  Scene::~Scene()
  {
    for(vk::Framebuffer &frame : frames)
    {
      device->destroyFramebuffer(frame);
    }

    device->destroyRenderPass(pass);
  }
}

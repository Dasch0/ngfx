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

  Scene::Scene(Context *pContext, SwapData *pSwap)
    : cam(pContext, pSwap->extent), c(pContext), s(pSwap), cmdCount(s->swapCount)
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
    for(size_t i = 0; i < cmdCount; i++)
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
    
    c->device.createDescriptorSetLayout(
        &layoutCI,
        nullptr,
        &descLayout);

    vk::PipelineLayoutCreateInfo pipelineLayoutCI(
        vk::PipelineLayoutCreateFlags(),
        1,
        &descLayout,
        0,
        nullptr);

    c->device.createPipelineLayout(
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

    createDescriptorPool();
    createDescriptorSets();
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
    
    c->device.createDescriptorPool(
       &poolInfo, 
       nullptr, 
       &descPool);
  }

  void Scene::createDescriptorSets(void)
  {
    vk::DescriptorSetAllocateInfo allocInfo(descPool,
                                            1,
                                            &descLayout);

    c->device.allocateDescriptorSets(&allocInfo, &descSet);

    vk::DescriptorBufferInfo buffInfo(
        cam.cam.localBuffer,
        0,
        sizeof(cam.cam.data));
    
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

    c->device.updateDescriptorSets(
        util::array_size(descWrite),
        descWrite,
        0,
        nullptr); 
  }
 
    // TODO:: parallelize command buffer creation
    void Scene::buildCommandBuffers(void)
    {
      vk::CommandBufferAllocateInfo allocInfo(
          c->cmdPool,
          vk::CommandBufferLevel::ePrimary,
          (uint)s->swapCount);
      
      c->device.allocateCommandBuffers(&allocInfo, cmdBuff);

      for (size_t i = 0; i < cmdCount; i++) {
        vk::CommandBufferBeginInfo beginInfo(
            vk::CommandBufferUsageFlags(),
            nullptr);

        vk::DeviceSize offsets[] = {0};

        // TODO: Fix weird code for clearValue
        // Currently requires two sub-classes to construct
        const std::array<float, 4> clearColorPrimative = {0.1f, 0.1f, 0.1f,
                                                          1.0f};
        vk::ClearColorValue clearColor(clearColorPrimative);
        const vk::ClearValue clearValue(clearColor);

        vk::RenderPassBeginInfo scenePassInfo(
            pass,
            frames[i],
            vk::Rect2D(
              vk::Offset2D(0, 0),
              s->extent),
            1,
            &clearValue);

        cmdBuff[i].beginRenderPass(
            scenePassInfo,
            vk::SubpassContents::eInline);
        cmdBuff[i].bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            pipeline);
        cmdBuff[i].bindVertexBuffers(
            0,
            1,
            &_envVertexBuffer.localBuffer,
            (const vk::DeviceSize *)offsets);
        cmdBuff[i].bindVertexBuffers(
            1,
            1,
            &_envInstanceBuffer.localBuffer,
            (const vk::DeviceSize *)offsets);
        cmdBuff[i].bindIndexBuffer(
            _envIndexBuffer.localBuffer,
            0,
            vk::IndexType::eUint16);
        cmdBuff[i].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            cameraArray.layout,
            0,
            1,
            &cameraArray.descSet,
            0,
            nullptr);
        cmdBuff[i].drawIndexed(
            util::array_size(testIndices),
            util::array_size(testInstances),
            0,
            0,
            0);
        cmdBuff[i].endRenderPass();

        cmdBuff[i].beginRenderPass(overlayPassInfo,
                                          vk::SubpassContents::eInline);

        cmdBuff[i].pushConstants(
            overlay.layout, vk::ShaderStageFlagBits::eVertex, 0,
            sizeof(OverlayTestOffset), (void *)&overlayOffset);

        cmdBuff[i].bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            overlay.pipeline);

        cmdBuff[i].bindVertexBuffers(
            0,
            1,
            &_overlayVertexBuffer.localBuffer,
            (const vk::DeviceSize *)offsets);

        cmdBuff[i].bindIndexBuffer(
            _overlayIndexBuffer.localBuffer,
            0,
            vk::IndexType::eUint16);
        cmdBuff[i].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            overlay.layout,
            0,
            1,
            &overlay.descSet,
            0,
            nullptr);
        cmdBuff[i].drawIndexed(
            util::array_size(overlayIndices),
            1,
            0,
            0,
            0);
        cmdBuff[i].endRenderPass();
        cmdBuff[i].end();
      }
    }
  Scene::~Scene()
  {
    for(vk::Framebuffer &frame : frames)
    {
      c->device.destroyFramebuffer(frame);
    }

    c->device.destroyRenderPass(pass);
    c->device.destroyPipelineLayout(layout);
    c->device.destroyPipeline(pipeline);


    c->device.freeDescriptorSets(descPool, descSet); 
    c->device.destroyDescriptorSetLayout(descLayout);
    c->device.destroyDescriptorPool(descPool);
  }
}

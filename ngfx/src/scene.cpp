#include "ngfx.hpp"
#include <vulkan/vulkan.hpp>

namespace ngfx
{
  Scene::Scene(Context *pContext)
    : c(pContext)
  {
//    // RenderPass
//    vk::AttachmentDescription
//      colorAttachment(vk::AttachmentDescriptionFlags(),
//                      s->format, 
//                      vk::SampleCountFlagBits::e1,
//                      vk::AttachmentLoadOp::eClear,
//                      vk::AttachmentStoreOp::eStore,
//                      vk::AttachmentLoadOp::eDontCare,
//                      vk::AttachmentStoreOp::eDontCare,
//                      vk::ImageLayout::eUndefined,
//                      vk::ImageLayout::ePresentSrcKHR);
//    vk::AttachmentReference colorAttachmentRef(
//        0,
//        vk::ImageLayout::eColorAttachmentOptimal);
//    
//    vk::SubpassDescription subpass(
//        vk::SubpassDescriptionFlags(),
//        vk::PipelineBindPoint::eGraphics,
//        0,
//        nullptr,
//        1,
//        &colorAttachmentRef,
//        nullptr,
//        nullptr,
//        0,
//        nullptr);
//    
//    vk::SubpassDependency subpassDependency(
//        0,
//        VK_SUBPASS_EXTERNAL,
//        vk::PipelineStageFlagBits::eColorAttachmentOutput,
//        vk::PipelineStageFlagBits::eColorAttachmentOutput,
//        vk::AccessFlags(),
//        vk::AccessFlagBits::eColorAttachmentWrite,
//        vk::DependencyFlags());
//    
//    vk::RenderPassCreateInfo renderPassCI(
//        vk::RenderPassCreateFlags(),
//        1,
//        &colorAttachment,
//        1,
//        &subpass,
//        1,
//        &subpassDependency);
//    
//    c->device.createRenderPass(&renderPassCI, nullptr, &pass);
//    
//    // Framebuffers
//    frames.resize(s->views.size());
//    for(size_t i = 0; i < s->views.size(); i++)
//    {
//      vk::ImageView attachments[] = {
//        s->views[i]
//      };
//
//      vk::FramebufferCreateInfo framebufferCI(
//          vk::FramebufferCreateFlags(),
//          pass,
//          1,
//          attachments,
//          s->extent.width,
//          s->extent.height,
//          1);
//      
//      c->device.createFramebuffer(
//          &framebufferCI,
//          nullptr,
//          &frames[i]);
//    }
//
//    //Descriptors & buffers
//    vk::DescriptorSetLayoutBinding bindings[] = {
//      vk::DescriptorSetLayoutBinding(
//         0,
//         vk::DescriptorType::eUniformBuffer,
//         1,
//         vk::ShaderStageFlagBits::eVertex, 
//         nullptr)
//    };
//    
//    vk::DescriptorSetLayoutCreateInfo layoutCI(
//        vk::DescriptorSetLayoutCreateFlags(),
//        util::array_size(bindings),
//        bindings); 
//    
//    device->createDescriptorSetLayout(
//        &layoutCI,
//        nullptr,
//        &descLayout);
//
//    vk::PipelineLayoutCreateInfo pipelineLayoutCI(
//        vk::PipelineLayoutCreateFlags(),
//        1,
//        &descLayout,
//        0,
//        nullptr);
//
//    device->createPipelineLayout(
//        &pipelineLayoutCI,
//        nullptr,
//        &layout);
//
//    util::buildPipeline(
//        &c->device,
//        s->extent,
//        binding,
//        util::array_size(binding),
//        attribute,
//        util::array_size(attribute),
//        "shaders/env_vert.spv",
//        "shaders/env_frag.spv",
//        false,
//        &layout,
//        &pass,
//        &c->pipelineCache,
//        &pipeline);
//
//    camBuffer.init();
//    createDescriptorPool();
//    createDescriptorSets();
//    camBuffer.stage(&cam.cam);
//
//    // TODO: Fix hack that stores queue here, prefer to restructure fastbuffer
//    q = &c->graphicsQueue;
//    camBuffer.blockingCopy(c->graphicsQueue);
  }

  void Scene::createDescriptorPool(void) {
//    vk::DescriptorPoolSize poolSize[] = {
//      vk::DescriptorPoolSize(
//          vk::DescriptorType::eUniformBuffer,
//          1)
//    };
//
//    vk::DescriptorPoolCreateInfo poolInfo(
//        vk::DescriptorPoolCreateFlags(),
//        1,
//        util::array_size(poolSize),
//        poolSize); 
//    
//    device->createDescriptorPool(
//       &poolInfo, 
//       nullptr, 
//       &descPool);
  }

  void Scene::createDescriptorSets(void)
  {
//    vk::DescriptorSetAllocateInfo allocInfo(descPool,
//                                            1,
//                                            &descLayout);
//
//    device->allocateDescriptorSets(&allocInfo, &descSet);
//
//    vk::DescriptorBufferInfo buffInfo(
//        camBuffer.localBuffer,
//        0,
//        sizeof(cam.cam));
//    
//    vk::WriteDescriptorSet descWrite[] = { 
//      vk::WriteDescriptorSet(
//          descSet,
//          0,
//          0,
//          1,
//          vk::DescriptorType::eUniformBuffer,
//          nullptr,
//          &buffInfo,
//          nullptr)
//    };
//
//    device->updateDescriptorSets(
//        util::array_size(descWrite),
//        descWrite,
//        0,
//        nullptr); 
  }


  void Scene::loadMeshes(Handle<Mesh> meshes)
  {
    vertexOffsets = alloc<vk::DeviceSize>(meshes.cnt);
    indexOffsets = alloc<vk::DeviceSize>(meshes.cnt);
    size_t vCnt = 0;
    size_t iCnt = 0;
    for(size_t i = 0; i < meshes.cnt; i++) {
      vertexOffsets[i] = vCnt * sizeof(Vertex);
      vCnt += meshes[i].vertices.cnt;
      indexOffsets[i] = iCnt * sizeof(Index);
      iCnt += meshes[i].indices.cnt;
    }
    vertices = alloc<Vertex>(vCnt);
    indices = alloc<Index>(iCnt);

    for(size_t i = 0; i < meshes.cnt;i++) {
       memcpy(
           vertices.ptr + vertexOffsets[i],
           meshes[i].vertices.ptr,
           meshes[i].vertices.cnt);
       memcpy(
           indices.ptr + indexOffsets[i],
           meshes[i].indices.ptr,
           meshes[i].indices.cnt);
    }

  }
  
  Scene::~Scene()
  {
    for(vk::Framebuffer &frame : frames)
    {
      c->device.destroyFramebuffer(frame);
    }

    c->device.destroyRenderPass(pass);
  }
}

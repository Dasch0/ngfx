#include "glm/fwd.hpp"
#include "ngfx.hpp"
#include <vulkan/vulkan.hpp>

namespace ngfx
{
  Scene::Scene(Context *pContext)
    : c(pContext), numTargets(0)
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
    vertices.cnt = vCnt;
    indices.cnt = iCnt;

    // Get mapped Handles to vertices/indices for host side loading of data
    vertexBuffer.init(sizeof(Vertex) * vCnt, vk::BufferUsageFlagBits::eVertexBuffer);
    vertices.ptr = (Vertex *) vertexBuffer.map();
    indexBuffer.init(sizeof(Index) * iCnt, vk::BufferUsageFlagBits::eIndexBuffer);
    indices.ptr = (Index *) indexBuffer.map();

    // Load mesh data into buffers
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
    // Copy buffers to GPU
    vertexBuffer.copy();
    indexBuffer.copy();
  }
  
  void Scene::initInstances(Handle<size_t> instanceCounts)
  {
    instanceOffsets = alloc<vk::DeviceSize>(instanceCounts.cnt);
    size_t iCnt = 0;
    for(size_t i = 0; i < instanceCounts.cnt; i++) {
      instanceOffsets[i] = iCnt;
      iCnt += instanceCounts[i];
    }
    instances.cnt = iCnt;
    instanceBuffer.init(
        sizeof(Instance) * iCnt,
        vk::BufferUsageFlagBits::eVertexBuffer);
    
    instances.ptr = (Instance *) instanceBuffer.map();
  }

  void Scene::initCameras(size_t count)
  {
    cameras = alloc<Camera>(count);
    viewProjectionBuffer.init(
        sizeof(glm::mat4) * count,
        vk::BufferUsageFlagBits::eStorageBuffer);
    
    viewProjections.ptr = (glm::mat4 *) viewProjectionBuffer.map();
    viewProjections.cnt = count;
  }

  void Scene::buildIndirectDrawCommands(void)
  {
    drawCmdBuffer.init(vertexOffsets.cnt, vk::BufferUsageFlagBits::eIndirectBuffer);
    drawCmds.ptr = (vk::DrawIndexedIndirectCommand *) drawCmdBuffer.map();
    drawCmds.cnt = vertexOffsets.cnt;
    for(size_t i = 0; i < drawCmds.cnt; i++) {
      drawCmds[i] = vk::DrawIndexedIndirectCommand(
          indexOffsets[i+1] - indexOffsets[i],
          instanceOffsets[i+1] - instanceOffsets[i],
          indexOffsets[i],
          vertexOffsets[i],
          instanceOffsets[i]);
    }
    drawCmdBuffer.copy();
  }

  void Scene::initTargets(size_t maxRenderTargets)
  {
    targets = alloc<RenderTarget>(maxRenderTargets);
    numTargets = 0;
  }

  void Scene::addTarget(uint32_t width, uint32_t height)
  {
    assert(numTargets < targets.cnt);
  
    vk::ImageCreateInfo imageInfo(
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(width, height, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment
        | vk::ImageUsageFlagBits::eSampled
        | vk::ImageUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive);

    vk::ImageCreateInfo colorInfo(
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(width, height, 1),
        1,
        1,
        c->msaaSamples,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment
        | vk::ImageUsageFlagBits::eTransientAttachment,
        vk::SharingMode::eExclusive);

    vk::ImageCreateInfo depthInfo(
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(width, height, 1),
        1,
        1,
        c->msaaSamples,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::SharingMode::eExclusive);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(
        c->allocator,
        (VkImageCreateInfo *) &imageInfo,
        &allocInfo,
        (VkImage *) &targets[numTargets].image,
        &targets[numTargets].imageAlloc,
        nullptr);
    vmaCreateImage(
        c->allocator,
        (VkImageCreateInfo *) &colorInfo,
        &allocInfo,
        (VkImage *) &targets[numTargets].color,
        &targets[numTargets].imageAlloc,
        nullptr);
    vmaCreateImage(
        c->allocator,
        (VkImageCreateInfo *) &depthInfo,
        &allocInfo,
        (VkImage *) &targets[numTargets].depth,
        &targets[numTargets].imageAlloc,
        nullptr);

    vk::ImageViewCreateInfo viewCI(
        vk::ImageViewCreateFlags(),
        targets[numTargets].image,
        vk::ImageViewType::e2D,
        vk::Format::eR8G8B8A8Srgb,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(
          vk::ImageAspectFlagBits::eColor,
          0, //baseMipLevel
          1, //levelCount
          0, //baseArrayLayer
          1)
        ); //layerCount

    c->device.createImageView(&viewCI, nullptr, &targets[numTargets].imageView);
    numTargets++;
  }

  void Scene::buildDefaultTargets(void)
  {
    for(size_t i = 0; i < cameras.cnt; i++) {
      addTarget(cameras[i].width, cameras[i].height);
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

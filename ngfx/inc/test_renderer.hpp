#ifndef NGFX_TESTRENDERER_H
#define NGFX_TESTRENDERER_H

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include "ngfx.hpp"
#include "config.hpp"
#include "util.hpp"
#include "context.hpp"
#include "swap_data.hpp"
#include "scene.hpp"
#include "pipeline.hpp"

// TODO: Docs
namespace ngfx
{
  const util::Vertex testVertices[] = {
    {{-0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}, {0.0f, 0.0f}},
    {{0.0f, 0.5f}, {0.9f, 0.9f, 0.9f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}, {1.0f, 1.0f}},
  };

  const uint16_t testIndices[] = {
    0, 1, 2, 0
  };

  const util::Instance testInstances[] = {
    {{2.0, 2.0}},
    {{2.0, 0.0}},
    {{2.0, -2.0}},
    {{0.0, 2.0}},
    {{0.0, 0.0}},
    {{0.0, -2.0}},
    {{-2.0, 2.0}},
    {{-2.0, 0.0}},
    {{-2.0, -2.0}},
  };
  const uint32_t kTestInstanceCount = 9;

  const util::Vertex overlayVertices[] = {
      {{-0.25f, -0.25f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      {{0.25f, -0.25f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{0.25f, 0.25f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      {{-0.25f, 0.25f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

  const uint16_t overlayIndices[] = {
    0, 1, 2, 2, 3, 0
  };

  struct OverlayTestOffset
  {
    glm::vec2 offset;
  } const overlayOffset = {{0.5, -0.5}};

  // TODO: clean up vertex input attribute and binding creation
  // TODO: replace with lightweight vector class TBD
  vk::VertexInputAttributeDescription attributeDesc[] = {
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

  vk::VertexInputBindingDescription bindingDesc[] = {
    vk::VertexInputBindingDescription(
        0,
        sizeof(util::Vertex),
        vk::VertexInputRate::eVertex),
    vk::VertexInputBindingDescription(
        0,
        sizeof(util::Instance),
        vk::VertexInputRate::eInstance)
  };
  
  class TestRenderer
  {
  public:
    Context c;
    SwapData swapData;
    Scene scene;

    TestRenderer()
      : c(), swapData(&c), scene(&c, &swapData), _currentFrame(0) {}

    void init(void)
    {
      _commandPool = util::createCommandPool(&c.device, c.qFamilies);
      buildOffscreenRenderPass();
      createTestOffscreenFbo();

      util::buildLayout(&c.device, sizeof(_envMvp), &_offscreenLayout);
      util::buildPipeline(
          &c.device,
          _offscreenFbo.extent,
          bindingDesc,
          util::array_size(bindingDesc),
          attributeDesc,
          util::array_size(attributeDesc),
          "shaders/env_vert.spv",
          "shaders/env_frag.spv",
          &_offscreenLayout,
          &_offscreenRenderPass,
          &c.pipelineCache,
          &_offscreenPipeline);

      util::buildLayout(&c.device, sizeof(_envMvp), &_envLayout);
      util::buildPipeline(
          &c.device,
          swapData.extent,
          bindingDesc,
          util::array_size(bindingDesc),
          attributeDesc,
          util::array_size(attributeDesc),
          "shaders/env_vert.spv",
          "shaders/env_frag.spv",
          &_envLayout,
          &swapData.renderPass,
          &c.pipelineCache,
          &_envPipeline);

      createEnvBuffers(); 
      util::buildLayout(&c.device, sizeof(glm::vec2), &_offscreenLayout);
      util::buildPipeline(
          &c.device,
          swapData.extent,
          bindingDesc,
          util::array_size(bindingDesc),
          attributeDesc,
          util::array_size(attributeDesc),
          "shaders/overlay_vert.spv",
          "shaders/overlay_frag.spv",
          &_overlayLayout,
          &swapData.renderPass,
          &c.pipelineCache,
          &_overlayPipeline);
 
      createOverlayDescriptorSetLayout();
      createTextureSampler();
      
      createOverlayBuffers();
      createDescriptorPool();
      createOverlayDescriptorSets();

      buildOffscreenCommandBuffer();
      buildCommandBuffers();

      // Create sync tools
      for (uint i = 0; i < ngfx::kMaxFramesInFlight; i++)
      {
        vk::SemaphoreCreateInfo semaphoreCI;
        c.device.createSemaphore(&semaphoreCI,
                                nullptr,
                                &_semaphores[i].imageAvailable);
        c.device.createSemaphore(&semaphoreCI,
                                nullptr,
                                &_semaphores[i].renderComplete);
        vk::FenceCreateInfo fenceCI(vk::FenceCreateFlagBits::eSignaled);
        c.device.createFence(&fenceCI, nullptr, &_inFlightFences[i]);
      }
      swapData.fences.resize(swapData.images.size(), vk::Fence(nullptr));
    }

    void renderTest()
    {
      testLoop();
    }
    ~TestRenderer(void) { cleanup(); }

  private:
    vk::PipelineLayout _envLayout;
    vk::Pipeline _envPipeline;
    vk::RenderPass _offscreenRenderPass; 
    vk::PipelineLayout _offscreenLayout;
    vk::Pipeline _offscreenPipeline;
    vk::PipelineLayout _overlayLayout;
    vk::Pipeline _overlayPipeline;
    
    vk::CommandPool _commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    vk::CommandBuffer offscreenCommandBuffer;

    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSet _descriptorSet;
    vk::DescriptorSetLayout _descLayouts;

    util::FastBuffer _overlayVertexBuffer;
    util::FastBuffer _overlayIndexBuffer;
    util::FastBuffer _envVertexBuffer;
    util::FastBuffer _envIndexBuffer;
    util::FastBuffer _envInstanceBuffer;
    glm::mat4 _envMvp;

    util::SemaphoreSet _semaphores[ngfx::kMaxFramesInFlight];
    vk::Fence _inFlightFences[ngfx::kMaxFramesInFlight];
    uint32_t _currentFrame = 0;

    util::Fbo _offscreenFbo;
    vk::Sampler _sampler;


    void testLoop(void)
    {
      double lastTime = glfwGetTime();
      int nbFrames = 0;
      drawOffscreenFrame();
      while (!glfwWindowShouldClose(c.window)) {

        // Measure speed
        double currentTime = glfwGetTime();
        nbFrames++;
        if ( currentTime - lastTime >= 1.0 )
        {
          printf("%f ms/frame: \n", 1000.0 / double(nbFrames));
          nbFrames = 0;
          lastTime += 1.0;
        }
        drawFrame();
      }
      c.device.waitIdle();
    }

    void cleanup(void)
    {
      // TODO: Fix destructor ordering/dependencies so this doesn't happen
      _overlayVertexBuffer.~FastBuffer();
      _overlayIndexBuffer.~FastBuffer();

      c.device.destroyDescriptorPool(_descriptorPool);
      c.device.destroyDescriptorSetLayout(_descLayouts);

      for (uint i = 0; i < ngfx::kMaxFramesInFlight; i++)
      {
        c.device.destroySemaphore(_semaphores[i].imageAvailable);
        c.device.destroySemaphore(_semaphores[i].renderComplete);
        c.device.destroyFence(_inFlightFences[i]);
      }

      c.device.destroyCommandPool(_commandPool);
      c.device.destroy();
      util::DestroyDebugUtilsMessengerEXT(c.instance, c.debugMessenger);
      vkDestroySurfaceKHR(c.instance, c.surface, nullptr);
      c.instance.destroy();

      glfwDestroyWindow(c.window);

      glfwTerminate();
    }

    void drawOffscreenFrame(void)
    {
      vk::SubmitInfo submitInfo(
            0,
            nullptr,
            nullptr,
            1,
            &offscreenCommandBuffer,
            0,
            nullptr);

      c.graphicsQueue.submit(1, &submitInfo, nullptr);
      c.graphicsQueue.waitIdle();
    }

    void drawFrame(void)
    {
      uint32_t imageIndex;
      { // Prepare frame
        c.device.waitForFences(
            1,
            &_inFlightFences[_currentFrame],
            true,
            UINT64_MAX);

        c.device.acquireNextImageKHR(swapData.swapchain, UINT64_MAX,
                                     _semaphores[_currentFrame].imageAvailable,
                                     vk::Fence(nullptr), &imageIndex);

        if (swapData.fences[imageIndex] != vk::Fence(nullptr))
        {
          c.device.waitForFences(1, &swapData.fences[imageIndex], true, UINT64_MAX);
        }
        swapData.fences[imageIndex] = _inFlightFences[_currentFrame];
        c.device.resetFences(1, (const vk::Fence *)&_inFlightFences[_currentFrame]);
      }
      { // Draw frame
        vk::PipelineStageFlags waitStages(
            vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo submitInfo(1, &_semaphores[_currentFrame].imageAvailable,
                                  &waitStages, 1, &commandBuffers[imageIndex], 1,
                                  &_semaphores[_currentFrame].renderComplete);
        c.graphicsQueue.submit(1, &submitInfo, _inFlightFences[_currentFrame]);
      }
      { // Present frame 
        vk::PresentInfoKHR presentInfo(
            1, &_semaphores[_currentFrame].renderComplete, 1, &swapData.swapchain,
            &imageIndex, nullptr);
    
        c.presentQueue.presentKHR(&presentInfo);
        _currentFrame = (_currentFrame + 1) % kMaxFramesInFlight;
      }
    }

    void buildOffscreenRenderPass(void)
    {
      vk::AttachmentDescription colorAttachment(
          vk::AttachmentDescriptionFlags(),
          vk::Format::eR8G8B8A8Srgb,
          vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eClear,
          vk::AttachmentStoreOp::eStore,
          vk::AttachmentLoadOp::eDontCare,
          vk::AttachmentStoreOp::eDontCare,
          vk::ImageLayout::eUndefined,
          vk::ImageLayout::eShaderReadOnlyOptimal);
      
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
      
      c.device.createRenderPass(
          &renderPassCI,
          nullptr,
          &_offscreenRenderPass);
    }
    
    void buildOffscreenCommandBuffer(void)
    {
      updateTestMvp(nullptr);
      vk::CommandBuffer *cmd = &offscreenCommandBuffer;
      vk::CommandBufferAllocateInfo allocInfo(_commandPool,
                                              vk::CommandBufferLevel::ePrimary,
                                              1);
      c.device.allocateCommandBuffers(&allocInfo, cmd);


      vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(),
                                             nullptr);
      vk::DeviceSize offsets[] = {0};

      // TODO: Fix weird code for clearValue
      // Currently requires two sub-classes to construct
      const std::array<float, 4> clearColorPrimative = {0.1f, 0.1f, 0.1f,
                                                        1.0f};
      vk::ClearColorValue clearColor(clearColorPrimative);
      const vk::ClearValue clearValue(clearColor);
      

      cmd->begin(beginInfo);

      vk::RenderPassBeginInfo envPassInfo(
          _offscreenRenderPass, _offscreenFbo.frame,
          vk::Rect2D(vk::Offset2D(0, 0), _offscreenFbo.extent), 1, &clearValue);

      cmd->beginRenderPass(envPassInfo,
          vk::SubpassContents::eInline);

      cmd->pushConstants(_offscreenLayout, vk::ShaderStageFlagBits::eVertex, 0,
                         sizeof(_envMvp), (void *)&_envMvp);

      cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, _offscreenPipeline);

      cmd->bindVertexBuffers(
          0,
          1,
          &_envVertexBuffer.localBuffer,
          (const vk::DeviceSize *) offsets);
      cmd->bindVertexBuffers(
          1,
          1,
          &_envInstanceBuffer.localBuffer,
          (const vk::DeviceSize *) offsets);


      cmd->bindIndexBuffer(
          _envIndexBuffer.localBuffer,
          0,
          vk::IndexType::eUint16);
      cmd->drawIndexed(
          util::array_size(testIndices),
          util::array_size(testInstances),
          0,
          0,
          0);
      cmd->endRenderPass();
      cmd->end();
    }

    // TODO:: parallelize command buffer creation
    void buildCommandBuffers(void)
    {
      commandBuffers.resize(swapData.framebuffers.size());
      vk::CommandBufferAllocateInfo allocInfo(_commandPool,
                                              vk::CommandBufferLevel::ePrimary,
                                              (uint)commandBuffers.size());
      c.device.allocateCommandBuffers(&allocInfo, commandBuffers.data());

      for (size_t i = 0; i < commandBuffers.size(); i++) {
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(),
                                             nullptr);

        vk::DeviceSize offsets[] = {0};

        // TODO: Fix weird code for clearValue
        // Currently requires two sub-classes to construct
        const std::array<float, 4> clearColorPrimative = {0.1f, 0.1f, 0.1f,
                                                          1.0f};
        vk::ClearColorValue clearColor(clearColorPrimative);
        const vk::ClearValue clearValue(clearColor);

        vk::RenderPassBeginInfo envPassInfo(
            swapData.renderPass,
            swapData.framebuffers[i],
            vk::Rect2D(
              vk::Offset2D(0, 0),
              swapData.extent),
            1,
            &clearValue);
        vk::RenderPassBeginInfo overlayPassInfo(
            swapData.renderPass,
            swapData.framebuffers[i],
            vk::Rect2D(
              vk::Offset2D(0, 0),
              swapData.extent),
            0,
            nullptr);


        commandBuffers[i].begin(beginInfo);
        commandBuffers[i].beginRenderPass(
            envPassInfo,
            vk::SubpassContents::eInline);
        commandBuffers[i].pushConstants(
            _envLayout,
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(_envMvp),
            (void *) &_envMvp);
        commandBuffers[i].bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            _envPipeline);
        commandBuffers[i].bindVertexBuffers(
            0,
            1,
            &_envVertexBuffer.localBuffer,
            (const vk::DeviceSize *)offsets);
        commandBuffers[i].bindVertexBuffers(
            1,
            1,
            &_envInstanceBuffer.localBuffer,
            (const vk::DeviceSize *)offsets);
        commandBuffers[i].bindIndexBuffer(
            _envIndexBuffer.localBuffer,
            0,
            vk::IndexType::eUint16);
        commandBuffers[i].drawIndexed(
            util::array_size(testIndices),
            util::array_size(testInstances),
            0,
            0,
            0);
        commandBuffers[i].endRenderPass();

        commandBuffers[i].beginRenderPass(overlayPassInfo,
                                          vk::SubpassContents::eInline);

        commandBuffers[i].pushConstants(
            _overlayLayout, vk::ShaderStageFlagBits::eVertex, 0,
            sizeof(OverlayTestOffset), (void *)&overlayOffset);

        commandBuffers[i].bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            _overlayPipeline);

        commandBuffers[i].bindVertexBuffers(
            0,
            1,
            &_overlayVertexBuffer.localBuffer,
            (const vk::DeviceSize *)offsets);

        commandBuffers[i].bindIndexBuffer(
            _overlayIndexBuffer.localBuffer,
            0,
            vk::IndexType::eUint16);
        commandBuffers[i].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            _overlayLayout,
            0,
            1,
            &_descriptorSet,
            0,
            nullptr);
        commandBuffers[i].drawIndexed(
            util::array_size(overlayIndices),
            1,
            0,
            0,
            0);
        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
      }
    }

    // TODO: use dedicated transfer queue, remove need for blocking copy
    void createOverlayBuffers(void)
    {
      _overlayVertexBuffer = util::FastBuffer(
          c.device, c.physicalDevice, _commandPool, sizeof(overlayVertices),
          vk::BufferUsageFlagBits::eVertexBuffer);

      _overlayVertexBuffer.init();
      _overlayVertexBuffer.stage((void *)overlayVertices);
      _overlayVertexBuffer.copy(c.graphicsQueue);

      _overlayIndexBuffer = util::FastBuffer(
          c.device, c.physicalDevice, _commandPool, sizeof(overlayIndices),
          vk::BufferUsageFlagBits::eIndexBuffer);
      _overlayIndexBuffer.init();
      _overlayIndexBuffer.stage((void *)overlayIndices);
      _overlayIndexBuffer.copy(c.graphicsQueue);
    }

    void createEnvBuffers(void)
    {
      _envVertexBuffer = util::FastBuffer(
          c.device, 
          c.physicalDevice, 
          _commandPool,
          sizeof(testVertices),
          vk::BufferUsageFlagBits::eVertexBuffer);
      _envVertexBuffer.init();
      _envVertexBuffer.stage((void *) testVertices);
      _envVertexBuffer.copy(c.graphicsQueue);
      
      _envIndexBuffer = util::FastBuffer(
          c.device, 
          c.physicalDevice, 
          _commandPool,
          sizeof(testIndices),
          vk::BufferUsageFlagBits::eIndexBuffer);
      _envIndexBuffer.init();
      _envIndexBuffer.stage((void *) testIndices);
      _envIndexBuffer.copy(c.graphicsQueue);

      _envInstanceBuffer = util::FastBuffer(
          c.device, 
          c.physicalDevice, 
          _commandPool,
          sizeof(testInstances),
          vk::BufferUsageFlagBits::eVertexBuffer);
      _envInstanceBuffer.init();
      _envInstanceBuffer.stage((void *) testInstances);
      _envInstanceBuffer.blockingCopy(c.graphicsQueue);
    }

    
    // TODO: Fix to work with push constants and keyboard input
    void updateTestMvp(vk::Semaphore waitSemaphore)
    {
      static auto startTime = std::chrono::high_resolution_clock::now();

      auto currentTime = std::chrono::high_resolution_clock::now();
      float time = std::chrono::duration<float, std::chrono::seconds::period>
          (currentTime - startTime).count();
      
      util::Mvp mvp;
      mvp.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                              glm::vec3(0.0f, 0.0f, 1.0f));
      mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 10.0f),
                             glm::vec3(0.0f, 0.0f, 0.0f),
                             glm::vec3(0.0f, 0.0f, 1.0f));
      mvp.proj = glm::perspective(
          glm::radians(90.0f),
          swapData.extent.width / (float)swapData.extent.height, 0.1f, 100.0f);
      mvp.proj[1][1] *= -1;

      _envMvp = mvp.proj * mvp.view * mvp.model;
    }

    // TODO: support variable number of descriptor sets
    // TODO: optimize maxsets and pool sizes, for now setting & forgetting
    void createDescriptorPool(void)
    {
      vk::DescriptorPoolSize poolSize[] = {
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1),
      };

      vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(),
                                            10,
                                            util::array_size(poolSize),
                                            poolSize);
      c.device.createDescriptorPool(&poolInfo, nullptr, &_descriptorPool);
    }

    void createOverlayDescriptorSetLayout(void)
    {
      vk::DescriptorSetLayoutBinding bindings[] = {
       vk::DescriptorSetLayoutBinding(
           0,
           vk::DescriptorType::eCombinedImageSampler,
           1,
           vk::ShaderStageFlagBits::eFragment, 
           nullptr)
      };
      vk::DescriptorSetLayoutCreateInfo
        layoutCI(vk::DescriptorSetLayoutCreateFlags(),
                 util::array_size(bindings),
                 bindings); 
      c.device.createDescriptorSetLayout(&layoutCI, nullptr, &_descLayouts);
    }

    void createOverlayDescriptorSets(void)
    {
      vk::DescriptorSetAllocateInfo allocInfo(_descriptorPool,
                                              1,
                                              &_descLayouts);

      c.device.allocateDescriptorSets(&allocInfo, &_descriptorSet);

      vk::DescriptorImageInfo imageInfo(_sampler,
                                        _offscreenFbo.view,
                                        vk::ImageLayout::eShaderReadOnlyOptimal);

      vk::WriteDescriptorSet descWrite[] = { 
        vk::WriteDescriptorSet(_descriptorSet,
                               0,
                               0,
                               1,
                               vk::DescriptorType::eCombinedImageSampler,
                               &imageInfo,
                               nullptr,
                               nullptr)
      };
      c.device.updateDescriptorSets(util::array_size(descWrite),
                                    descWrite,
                                    0,
                                    nullptr);
    }

    void createTestOffscreenFbo(void)
    {
      uint w = 256;
      uint h = 256;
      vk::Image *i = &_offscreenFbo.image;
      vk::DeviceMemory *m = &_offscreenFbo.mem;
      vk::ImageView *v = &_offscreenFbo.view;
      vk::Framebuffer *f = &_offscreenFbo.frame;

      _offscreenFbo.extent = vk::Extent2D(w, h);
      vk::ImageCreateInfo imageCI(vk::ImageCreateFlags(),
                                  vk::ImageType::e2D,
                                  vk::Format::eR8G8B8A8Srgb,
                                  vk::Extent3D(w, h, 1),
                                  1,
                                  1,
                                  vk::SampleCountFlagBits::e1,
                                  vk::ImageTiling::eOptimal,
                                  vk::ImageUsageFlagBits::eColorAttachment
                                  | vk::ImageUsageFlagBits::eSampled
                                  | vk::ImageUsageFlagBits::eTransferSrc,
                                  vk::SharingMode::eExclusive);
      c.device.createImage(&imageCI, nullptr, i);

      vk::MemoryRequirements memReqs;
      c.device.getImageMemoryRequirements(*i, &memReqs);
      auto memType =
          util::findMemoryType(c.physicalDevice, memReqs.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);
      vk::MemoryAllocateInfo allocInfo(memReqs.size, memType);
      c.device.allocateMemory(&allocInfo, nullptr, m);
      c.device.bindImageMemory(*i, *m, 0);

      vk::ImageViewCreateInfo viewCI(vk::ImageViewCreateFlags(),
                                     *i,
                                     vk::ImageViewType::e2D,
                                     vk::Format::eR8G8B8A8Srgb,
                                     vk::ComponentMapping(),
                                     vk::ImageSubresourceRange(
                                     vk::ImageAspectFlagBits::eColor,
                                     0, //baseMipLevel
                                     1, //levelCount
                                     0, //baseArrayLayer
                                     1) //layerCount
                                     );
      c.device.createImageView(&viewCI, nullptr, v);

      // Framebuffer
      vk::FramebufferCreateInfo framebufferCI(vk::FramebufferCreateFlags(),
                                              _offscreenRenderPass, 1, v, w, h,
                                              1); // Layer
      c.device.createFramebuffer(&framebufferCI, nullptr, f);
    }

    void createTextureSampler()
    {
      vk::SamplerCreateInfo samplerCI(vk::SamplerCreateFlags(),
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
      c.device.createSampler(&samplerCI, nullptr, &_sampler);
    }
  };
}

#endif //NGFX_TESTRENDERER_H

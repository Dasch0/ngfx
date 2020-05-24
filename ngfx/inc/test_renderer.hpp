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


// TODO: Docs
namespace ngfx
{
  class TestRenderer {
  public:
    Context c;
    SwapData swapData;

    TestRenderer() : c(), swapData(&c), _currentFrame(0) {}

    void init(void)
    {
      initVulkan();
    }
    void renderTest()
    {
      testLoop();
    }
    ~TestRenderer(void) { cleanup(); }

  private:
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _graphicsPipeline;

    vk::CommandPool _commandPool;
    std::vector<vk::CommandBuffer> _commandBuffers;
    
    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSet _descriptorSet;
    vk::DescriptorSetLayout _descLayouts;

    util::FastBuffer _vertexBuffer;
    util::FastBuffer _indexBuffer;
    util::FastBuffer _mvpBuffer;
    util::FastBuffer _instanceBuffer;

    util::SemaphoreSet _semaphores[ngfx::kMaxFramesInFlight];
    vk::Fence _inFlightFences[ngfx::kMaxFramesInFlight];
    uint32_t _currentFrame = 0;

    util::Fbo _offscreenFbo;
    vk::Sampler _sampler;


    void initVulkan(void)
    {
      createTestDescriptorSetLayout();
      createTextureSampler();
      createTestOffscreenFbo();
      buildTestGraphicsPipeline();
      _commandPool = util::createCommandPool(c.device, c.qFamilies);
      createTestBuffers();
      createDescriptorPool();
      createTestDescriptorSets();
      buildCommandBuffers();

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

    // TODO: pull loop out of Context class
    void testLoop(void)
    {
      double lastTime = glfwGetTime();
      int nbFrames = 0;
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
      _vertexBuffer.~FastBuffer();
      _indexBuffer.~FastBuffer();
      _mvpBuffer.~FastBuffer();
      _instanceBuffer.~FastBuffer();

      c.device.destroyDescriptorPool(_descriptorPool);

      cleanupSwapchain();
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

    void drawFrame()
    {
      //validateSwapchain();
      updateTestMvp(_semaphores[_currentFrame].renderComplete);
      c.device.waitForFences(1, &_inFlightFences[_currentFrame], true, UINT64_MAX);
      
      uint32_t imageIndex;
      c.device.acquireNextImageKHR(swapData.swapchain, UINT64_MAX,
                                   _semaphores[_currentFrame].imageAvailable,
                                   vk::Fence(nullptr), &imageIndex);

      if (swapData.fences[imageIndex] != vk::Fence(nullptr)) {
        c.device.waitForFences(1, &swapData.fences[imageIndex], true,
                               UINT64_MAX);
      }
      swapData.fences[imageIndex] = _inFlightFences[_currentFrame];

      c.device.resetFences(1, (const vk::Fence *)&_inFlightFences[_currentFrame]);

      vk::PipelineStageFlags waitStages(vk::PipelineStageFlagBits::eColorAttachmentOutput);
      vk::SubmitInfo submitInfo(1,
                                &_semaphores[_currentFrame].imageAvailable,
                                &waitStages,
                                1,
                                &_commandBuffers[imageIndex],
                                1,
                                &_semaphores[_currentFrame].renderComplete);
      c.graphicsQueue.submit(1, &submitInfo, _inFlightFences[_currentFrame]);

      vk::PresentInfoKHR presentInfo(
          1, &_semaphores[_currentFrame].renderComplete, 1, &swapData.swapchain,
          &imageIndex, nullptr);

      c.presentQueue.presentKHR(&presentInfo);
      _currentFrame = (_currentFrame + 1) % kMaxFramesInFlight;
    }

    void cleanupSwapchain(void)
    {
      for (auto fb : swapData.framebuffers) {
        c.device.destroyFramebuffer(fb);
      }
      c.device.freeCommandBuffers(_commandPool,
                                 (uint) _commandBuffers.size(),
                                 (const vk::CommandBuffer *)_commandBuffers.data());
      c.device.destroyPipeline(_graphicsPipeline);
      c.device.destroyRenderPass(swapData.renderPass);
      c.device.destroyPipelineLayout(_pipelineLayout);
      for (auto view : swapData.views)
        c.device.destroyImageView(view);
      c.device.destroySwapchainKHR(swapData.swapchain);
    }

    void validateSwapchain(void)
    {
      int width, height;
      glfwGetWindowSize(c.window, &width, &height);

      if (width == (int)c.swapInfo.capabilites.currentExtent.width &&
          height == (int)c.swapInfo.capabilites.currentExtent.height) {
        return;
      }

      c.device.waitIdle();

      cleanupSwapchain();
      util::querySwapchainSupport(&c.physicalDevice, &c.surface, &c.swapInfo);

      // TODO:: improve rebuilding swapchain to avoid copy constructor
      swapData = SwapData(&c);

      buildTestGraphicsPipeline();

      _commandPool = util::createCommandPool(c.device, c.qFamilies);

      buildCommandBuffers();
    }

    // TODO: dynamic state on viewport/scissor to speed up resize
    // Should implement working resize code first
    // TODO: Investigate whether there is any performance benefit to
    // using derivatives with any vendor, for now just using a single cache
    void buildTestGraphicsPipeline(void)
    {
      auto vertShaderCode = util::readFile("shaders/vert.spv");
      auto fragShaderCode = util::readFile("shaders/frag.spv");

      vk::ShaderModule vertModule = util::createShaderModule(c.device, vertShaderCode);
      vk::ShaderModule fragModule = util::createShaderModule(c.device, fragShaderCode);

      vk::PipelineShaderStageCreateInfo vertStageCI(vk::PipelineShaderStageCreateFlags(),
                                                    vk::ShaderStageFlagBits::eVertex,
                                                    vertModule,
                                                    "main");

      vk::PipelineShaderStageCreateInfo fragStageCI(vk::PipelineShaderStageCreateFlags(),
                                                    vk::ShaderStageFlagBits::eFragment,
                                                    fragModule,
                                                    "main");

      vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertStageCI,
        fragStageCI
      };

      vk::VertexInputBindingDescription bindingDesc[] = {
        util::Vertex::getBindingDescription(),
        util::Instance::getBindingDescription()
      };

      // TODO: clean up vertex input attribute and binding creation
      vk::VertexInputAttributeDescription attributeDesc[] = {
        // Per Vertex data
        vk::VertexInputAttributeDescription(0,
                                            0,
                                            vk::Format::eR32G32Sfloat,
                                            offsetof(util::Vertex, pos)),
        vk::VertexInputAttributeDescription(1,
                                            0,
                                            vk::Format::eR32G32B32Sfloat,
                                            offsetof(util::Vertex, color)),
        vk::VertexInputAttributeDescription(2,
                                            0,
                                            vk::Format::eR32G32Sfloat,
                                            offsetof(util::Vertex, texCoord)), 
        // Per Instance data
        vk::VertexInputAttributeDescription(3,
                                            1,
                                            vk::Format::eR32G32Sfloat,
                                            offsetof(util::Instance, pos)),
      };

      vk::PipelineVertexInputStateCreateInfo
          vertexInputCI(vk::PipelineVertexInputStateCreateFlags(),
                        2,
                        bindingDesc,
                        4,
                        attributeDesc);

      vk::PipelineInputAssemblyStateCreateInfo
          inputAssembly(vk::PipelineInputAssemblyStateCreateFlags(),
                        vk::PrimitiveTopology::eLineStrip,
                        false);

      vk::Viewport viewport(0.0f, 0.0f, swapData.extent.width,
                            swapData.extent.height, 0.0, 1.0);

      vk::Rect2D scissor(vk::Offset2D(0, 0), swapData.extent);

      vk::PipelineViewportStateCreateInfo
          viewportCI(vk::PipelineViewportStateCreateFlags(),
                     1,
                     &viewport,
                     1,
                     &scissor);

      vk::PipelineRasterizationStateCreateInfo
          rasterizerCI(vk::PipelineRasterizationStateCreateFlags(),
                     false,
                     false,
                     vk::PolygonMode::eFill,
                     vk::CullModeFlagBits::eBack,
                     vk::FrontFace::eCounterClockwise,
                     false,
                     0.0f,
                     0.0f,
                     0.0f,
                     1.0f);

      vk::PipelineMultisampleStateCreateInfo
          multisamplingCI(vk::PipelineMultisampleStateCreateFlags(),
                        vk::SampleCountFlagBits::e1,
                        false,
                        1.0f,
                        nullptr,
                        false,
                        false);

      vk::PipelineColorBlendAttachmentState
          colorBlendAttachment(false,
                               vk::BlendFactor::eOne,
                               vk::BlendFactor::eZero,
                               vk::BlendOp::eAdd,
                               vk::BlendFactor::eOne,
                               vk::BlendFactor::eZero,
                               vk::BlendOp::eAdd,
                               vk::ColorComponentFlagBits::eR
                               | vk::ColorComponentFlagBits::eG
                               | vk::ColorComponentFlagBits::eB
                               | vk::ColorComponentFlagBits::eA);

      vk::PipelineColorBlendStateCreateInfo
          colorBlendingCI(vk::PipelineColorBlendStateCreateFlags(),
                        false,
                        vk::LogicOp::eCopy,
                        1,
                        &colorBlendAttachment);

      vk::DynamicState dynamicStates[] =
      {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
      };

      vk::PipelineDynamicStateCreateInfo
          dynamicStateCI(vk::PipelineDynamicStateCreateFlags(),
                         2,
                         dynamicStates);

      vk::PipelineLayoutCreateInfo 
        pipelineLayoutCI(vk::PipelineLayoutCreateFlags(),
                         1,
                         &_descLayouts,
                         0,
                         nullptr);

      c.device.createPipelineLayout(&pipelineLayoutCI, nullptr, &_pipelineLayout);

      vk::GraphicsPipelineCreateInfo pipelineCI(vk::PipelineCreateFlags(),
                                                2,
                                                shaderStages,
                                                &vertexInputCI,
                                                &inputAssembly,
                                                nullptr,
                                                &viewportCI,
                                                &rasterizerCI,
                                                &multisamplingCI,
                                                nullptr,
                                                &colorBlendingCI,
                                                nullptr,
                                                _pipelineLayout,
                                                swapData.renderPass,
                                                0,
                                                nullptr,
                                                -1);

      c.device.createGraphicsPipelines(c.pipelineCache,
                                       1,
                                       &pipelineCI,
                                       nullptr,
                                       &_graphicsPipeline);

      c.device.destroyShaderModule(vertModule);
      c.device.destroyShaderModule(fragModule);
    }

    void buildCommandBuffers(void)
    {
      _commandBuffers.resize(swapData.framebuffers.size());
      vk::CommandBufferAllocateInfo allocInfo(_commandPool,
                                              vk::CommandBufferLevel::ePrimary,
                                              (uint) _commandBuffers.size());
      c.device.allocateCommandBuffers(&allocInfo, _commandBuffers.data());

      for (size_t i = 0; i < _commandBuffers.size(); i++)
      {
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(),
                                             nullptr);
        _commandBuffers[i].begin(beginInfo);

        // TODO: Fix weird code for clearValue
        // Currently requires two sub-classes to construct
        const std::array<float, 4> clearColorPrimative = {
          0.1f,
          0.1f,
          0.1f,
          1.0f
        };
        vk::ClearColorValue clearColor(clearColorPrimative);
        const vk::ClearValue clearValue(clearColor);
        vk::RenderPassBeginInfo renderPassInfo(
            swapData.renderPass, swapData.framebuffers[i],
            vk::Rect2D(vk::Offset2D(0, 0), swapData.extent), 1, &clearValue);
        _commandBuffers[i].beginRenderPass(renderPassInfo,
                                           vk::SubpassContents::eInline);
        _commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics,
                                        _graphicsPipeline);

        vk::DeviceSize offsets[] = {0};

        _commandBuffers[i].bindVertexBuffers(0,
                                             1,
                                             &_vertexBuffer.localBuffer,
                                             (const vk::DeviceSize *) offsets);
        _commandBuffers[i].bindVertexBuffers(1,
                                             1,
                                             &_instanceBuffer.localBuffer,
                                             (const vk::DeviceSize *) offsets);
        _commandBuffers[i].bindIndexBuffer(_indexBuffer.localBuffer,
                                           0,
                                            vk::IndexType::eUint16);
        _commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                              _pipelineLayout,
                                              0,
                                              1,
                                              &_descriptorSet,
                                              0,
                                              nullptr);
        _commandBuffers[i].drawIndexed(4, util::kTestInstanceCount, 0, 0, 0);
        _commandBuffers[i].endRenderPass();
        _commandBuffers[i].end();
      }
    }

    // TODO: use dedicated transfer queue, remove need for blocking copy
    void createTestBuffers(void)
    {
      vk::DeviceSize vertexSize = sizeof(util::testVertices);
      vk::DeviceSize indexSize = sizeof(util::testIndices);
      vk::DeviceSize mvpSize = sizeof(util::Mvp);
      vk::DeviceSize instanceSize = sizeof(util::testInstances);

      _vertexBuffer =
          util::FastBuffer(c.device, c.physicalDevice, _commandPool, vertexSize,
                           vk::BufferUsageFlagBits::eVertexBuffer);
      _vertexBuffer.init();
      _vertexBuffer.stage((void *)util::testVertices);
      _vertexBuffer.copy(c.graphicsQueue);

      _indexBuffer =
          util::FastBuffer(c.device, c.physicalDevice, _commandPool, indexSize,
                           vk::BufferUsageFlagBits::eIndexBuffer);
      _indexBuffer.init();
      _indexBuffer.stage((void *)util::testIndices);
      _indexBuffer.copy(c.graphicsQueue);

      _mvpBuffer =
          util::FastBuffer(c.device, c.physicalDevice, _commandPool, mvpSize,
                           vk::BufferUsageFlagBits::eUniformBuffer);
      _mvpBuffer.init();

      _instanceBuffer = util::FastBuffer(
          c.device, c.physicalDevice, _commandPool, instanceSize,
          vk::BufferUsageFlagBits::eVertexBuffer);
      _instanceBuffer.init();
      _instanceBuffer.stage((void *)util::testInstances);
      // Last copy is blocking
      _instanceBuffer.blockingCopy(c.graphicsQueue);
    }
    
    // TODO: Use waitSemaphore
    void updateTestMvp(vk::Semaphore waitSemaphore) {
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
      
      _mvpBuffer.stage(&mvp);
      _mvpBuffer.blockingCopy(c.graphicsQueue);
    }

    // TODO: support variable number of descriptor sets
    void createDescriptorPool(void)
    {
      vk::DescriptorPoolSize poolSize[] = {
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1),
      };

      vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(),
                                            2, // TODO: Learn what maxSets is used for
                                            2,
                                            poolSize);
      c.device.createDescriptorPool(&poolInfo, nullptr, &_descriptorPool);
    }

    void createTestDescriptorSetLayout(void)
    {
      vk::DescriptorSetLayoutBinding bindings[] = {
        vk::DescriptorSetLayoutBinding
                           (0,
                           vk::DescriptorType::eUniformBuffer,
                           1,
                           vk::ShaderStageFlagBits::eVertex,
                           nullptr),
        vk::DescriptorSetLayoutBinding 
                               (1,
                               vk::DescriptorType::eCombinedImageSampler,
                               1,
                               vk::ShaderStageFlagBits::eFragment, 
                               nullptr)
      };
      vk::DescriptorSetLayoutCreateInfo
        layoutCI(vk::DescriptorSetLayoutCreateFlags(),
                 2,
                 bindings); 
      c.device.createDescriptorSetLayout(&layoutCI, nullptr, &_descLayouts);
    }

    void createTestDescriptorSets(void)
    {
      vk::DescriptorSetAllocateInfo allocInfo(_descriptorPool,
                                              1,
                                              &_descLayouts);

      c.device.allocateDescriptorSets(&allocInfo, &_descriptorSet);

      vk::DescriptorBufferInfo bufferInfo(_mvpBuffer.localBuffer,
                               0,
                               sizeof(util::Mvp));
      vk::DescriptorImageInfo imageInfo(_sampler,
                                        _offscreenFbo.view,
                                        vk::ImageLayout::eShaderReadOnlyOptimal);

      vk::WriteDescriptorSet descWrite[] = { 
        vk::WriteDescriptorSet(_descriptorSet,
                               0,
                               0,
                               1,
                               vk::DescriptorType::eUniformBuffer,
                               nullptr,
                               &bufferInfo,
                               nullptr),
        vk::WriteDescriptorSet(_descriptorSet,
                               1,
                               0,
                               1,
                               vk::DescriptorType::eCombinedImageSampler,
                               &imageInfo,
                               nullptr,
                               nullptr)
      };

      c.device.updateDescriptorSets(util::array_size(descWrite), descWrite, 0, nullptr);
    }

    void createTestOffscreenFbo(void)
    {
      uint w = 256;
      uint h = 256;
      vk::Image *i = &_offscreenFbo.image;
      vk::DeviceMemory *m = &_offscreenFbo.mem;
      vk::ImageView *v = &_offscreenFbo.view;

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
    }

    void createTextureSampler()
    {
      vk::SamplerCreateInfo samplerCI(vk::SamplerCreateFlags(),
                                      vk::Filter::eLinear,
                                      vk::Filter::eLinear,
                                      vk::SamplerMipmapMode::eLinear,
                                      vk::SamplerAddressMode::eRepeat,
                                      vk::SamplerAddressMode::eRepeat,
                                      vk::SamplerAddressMode::eRepeat,
                                      0.0f,
                                      false,
                                      1.0f, // TODO: support anisotropy levels
                                      false,
                                      vk::CompareOp::eAlways,
                                      0.0f,
                                      0.0f,
                                      vk::BorderColor::eIntOpaqueWhite,
                                      false);
      c.device.createSampler(&samplerCI, nullptr, &_sampler);
    }
  };
}
#endif // CONTEXT_H   

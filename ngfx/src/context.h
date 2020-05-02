#ifndef CONTEXT_H
#define CONTEXT_H

#include "ngfx.h"
#include "config.h"
#include "util.h"

// TODO: Docs
// Main class for rendering
namespace ngfx
{
  class Context
  {
  public:
    static const uint32_t kWidth = 800;
    static const uint32_t kHeight = 600;
    // TODO: implement proper resizability
    bool resizable;
    bool vsync;

    Context() : resizable(false), vsync(true), _currentFrame(0) {}

    void init(void)
    {
      initWindow(kWidth, kHeight);
      initVulkan();
    }
    void renderTest()
    {

      testLoop();
    }
    ~Context(void)
    {
      cleanup();
    }

  private:
    // TODO: use hpp class for extension based types if possible
    GLFWwindow* _window;
    vk::Instance _instance;
    VkDebugUtilsMessengerEXT _primativeDebugMessenger;
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::SurfaceKHR _surface;
    VkSurfaceKHR _primativeSurface;

    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;
    util::QueueFamilyIndices _qFamilies;
    vk::Queue _presentQueue;
    vk::Queue _graphicsQueue;
    util::SwapchainSupportDetails _swapchainInfo;
    util::SwapchainData _swapData;

    vk::PipelineLayout _pipelineLayout;
    vk::RenderPass _renderPass;
    vk::Pipeline _graphicsPipeline;

    vk::CommandPool _commandPool;
    std::vector<vk::CommandBuffer> _commandBuffers;

    // TODO: Create utils struct for vertex buffer
    util::FastBuffer _vertexBuffer;
    util::FastBuffer _indexBuffer;
    util::FastBuffer _uniformBuffer;
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingMemory;


    util::SemaphoreSet _semaphores[ngfx::kMaxFramesInFlight];
    vk::Fence _inFlightFences[ngfx::kMaxFramesInFlight];
    uint32_t _currentFrame;

    void initWindow(int width, int height)
    {
      glfwInit();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, resizable);
      _window = glfwCreateWindow(width,
                     height,
                     "ngfx",
                     nullptr,
                     nullptr);
      glfwSetWindowUserPointer(_window, this);
    }

    // TODO: Evaluate alternative to copy constructors throughout
    // Search "TODO: Fix Copy Constructor" for related code
    void initVulkan(void)
    {
      _instance = util::createInstance("test", "ngfx", VK_API_VERSION_1_1);
      util::createDebugMessenger(static_cast<VkInstance>(_instance),
                                 &_primativeDebugMessenger);
      // TODO: Fix need for primatives here & for surface
      _debugMessenger = vk::DebugUtilsMessengerEXT(_primativeDebugMessenger);
      glfwCreateWindowSurface(_instance, _window, nullptr, &_primativeSurface);
      _surface = vk::SurfaceKHR(_primativeSurface);
      _physicalDevice = util::pickPhysicalDevice(_instance, _surface);
      _qFamilies = util::findQueueFamilies(_physicalDevice, _surface);
      _device = util::createLogicalDevice(_physicalDevice, _qFamilies);
      _graphicsQueue = _device.getQueue(_qFamilies.graphicsFamily.value(), 0);
      _presentQueue = _device.getQueue(_qFamilies.presentFamily.value(), 0);
      _swapchainInfo = util::querySwapchainSupport(_physicalDevice, _surface);
      util::createSwapchain(&_swapData,
                            _qFamilies,
                            _device,
                            _surface,
                            _swapchainInfo,
                            kWidth,
                            kHeight);

      buildTriangleRenderPass();
      buildTriangleGraphicsPipeline();

      util::createFramebuffers(&_swapData, _device, _renderPass);
      _commandPool = util::createCommandPool(_device, _qFamilies);

      createTestBuffers();
      buildCommandBuffers();

      for (uint i = 0; i < ngfx::kMaxFramesInFlight; i++)
      {
        _semaphores[i].imageAvailable = _device.createSemaphore(vk::SemaphoreCreateInfo()).value;
        _semaphores[i].renderComplete = _device.createSemaphore(vk::SemaphoreCreateInfo()).value;
        vk::FenceCreateInfo fenceCI(vk::FenceCreateFlagBits::eSignaled);
        _inFlightFences[i] = _device.createFence(fenceCI).value;
      }
      _swapData.fences.resize(_swapData.images.size(), vk::Fence(nullptr));


    }

    // TODO: pull loop out of Context class
    void testLoop(void)
    {
      double lastTime = glfwGetTime();
      int nbFrames = 0;
      while (!glfwWindowShouldClose(_window))
      {
        glfwPollEvents();
        // Measure speed
        double currentTime = glfwGetTime();
        nbFrames++;
        if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1 sec ago
          // printf and reset timer
          printf("%f ms/frame: \n", 1000.0/double(nbFrames));
          nbFrames = 0;
          lastTime += 1.0;
        }
        drawFrame();
      }
      _device.waitIdle();
    }

    void cleanup()
    {
      // TODO: Fix destructor ordering/dependencies so this doesn't happen
      _vertexBuffer.~FastBuffer();
      _indexBuffer.~FastBuffer();
      _uniformBuffer.~FastBuffer();

      cleanupSwapchain();

      for (uint i = 0; i < ngfx::kMaxFramesInFlight; i++)
      {
        _device.destroySemaphore(_semaphores[i].imageAvailable);
        _device.destroySemaphore(_semaphores[i].renderComplete);
        _device.destroyFence(_inFlightFences[i]);
      }

      _device.destroyCommandPool(_commandPool);
      _device.destroy();
      util::DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger);
      vkDestroySurfaceKHR(_instance, _surface, nullptr);
      _instance.destroy();

      glfwDestroyWindow(_window);

      glfwTerminate();
    }

    void drawFrame()
    {
      //validateSwapchain();
      _device.waitForFences(1, &_inFlightFences[_currentFrame], true, UINT64_MAX);

      uint32_t imageIndex;
      _device.acquireNextImageKHR(_swapData.swapchain,
                                  UINT64_MAX,
                                  _semaphores[_currentFrame].imageAvailable,
                                  vk::Fence(nullptr),
                                  &imageIndex);

      if (_swapData.fences[imageIndex] != vk::Fence(nullptr)) {
          _device.waitForFences(1, &_swapData.fences[imageIndex], true, UINT64_MAX);
      }
      _swapData.fences[imageIndex] = _inFlightFences[_currentFrame];

      _device.resetFences(1, &_inFlightFences[_currentFrame]);

      vk::PipelineStageFlags waitStages(vk::PipelineStageFlagBits::eColorAttachmentOutput);
      vk::SubmitInfo submitInfo(1,
                                &_semaphores[_currentFrame].imageAvailable,
                                &waitStages,
                                1,
                                &_commandBuffers[imageIndex],
                                1,
                                &_semaphores[_currentFrame].renderComplete);
      _graphicsQueue.submit(1, &submitInfo, _inFlightFences[_currentFrame]);

      vk::PresentInfoKHR presentInfo(1,
                                     &_semaphores[_currentFrame].renderComplete,
                                     1,
                                     &_swapData.swapchain,
                                     &imageIndex,
                                     nullptr);

      _presentQueue.presentKHR(&presentInfo);
    }

    void cleanupSwapchain(void)
    {
      for (auto fb : _swapData.framebuffers)
      {
        _device.destroyFramebuffer(fb);
      }
      _device.freeCommandBuffers(_commandPool,
                                 (uint) _commandBuffers.size(),
                                 _commandBuffers.data());
      _device.destroyPipeline(_graphicsPipeline);
      _device.destroyRenderPass(_renderPass);
      _device.destroyPipelineLayout(_pipelineLayout);
      for (auto view : _swapData.views) _device.destroyImageView(view);
      _device.destroySwapchainKHR(_swapData.swapchain);
    }

    void validateSwapchain(void)
    {
      int width, height;
      glfwGetWindowSize(_window, &width, &height);

      if (width == (int) _swapchainInfo.capabilites.currentExtent.width
          && height == (int) _swapchainInfo.capabilites.currentExtent.height)
      {
        return;
      }

      _device.waitIdle();

      cleanupSwapchain();
      _swapchainInfo = util::querySwapchainSupport(_physicalDevice, _surface);

      util::createSwapchain(&_swapData,
                            _qFamilies,
                            _device,
                            _surface,
                            _swapchainInfo,
                            (uint) width,
                            (uint) height);

      buildTriangleRenderPass();
      buildTriangleGraphicsPipeline();

      util::createFramebuffers(&_swapData, _device, _renderPass);
      _commandPool = util::createCommandPool(_device, _qFamilies);

      buildCommandBuffers();
    }

    // TODO: dynamic state on viewport/scissor to speed up resize
    void buildTriangleGraphicsPipeline(void)
    {
      auto vertShaderCode = util::readFile("shaders/vert.spv");
      auto fragShaderCode = util::readFile("shaders/frag.spv");

      vk::ShaderModule vertModule = util::createShaderModule(_device, vertShaderCode);
      vk::ShaderModule fragModule = util::createShaderModule(_device, fragShaderCode);

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

      auto bindingDesc = util::Vertex::getBindingDescription();
      auto attributeDesc = util::Vertex::getAttributeDescriptions();

      vk::PipelineVertexInputStateCreateInfo
          vertexInputCI(vk::PipelineVertexInputStateCreateFlags(),
                        1,
                        &bindingDesc,
                        (uint) attributeDesc.size(),
                        attributeDesc.data());

      vk::PipelineInputAssemblyStateCreateInfo
          inputAssembly(vk::PipelineInputAssemblyStateCreateFlags(),
                        vk::PrimitiveTopology::eTriangleList,
                        false);

      vk::Viewport viewport(0.0f,
                            0.0f,
                            _swapData.extent.width,
                            _swapData.extent.height,
                            0.0,
                            1.0);

      vk::Rect2D scissor(vk::Offset2D(0, 0), _swapData.extent);

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
                     vk::FrontFace::eClockwise,
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

      vk::PipelineLayoutCreateInfo pipelineLayoutCI(vk::PipelineLayoutCreateFlags(),
                                   0,
                                   nullptr,
                                   0,
                                   nullptr);

      _pipelineLayout = _device.createPipelineLayout(pipelineLayoutCI).value;

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
                                                _renderPass,
                                                0,
                                                nullptr,
                                                -1);

      _graphicsPipeline = _device.createGraphicsPipeline(nullptr, pipelineCI).value;

      _device.destroyShaderModule(vertModule);
      _device.destroyShaderModule(fragModule);
    }

    void buildTriangleRenderPass(void)
    {
      vk::AttachmentDescription colorAttachment(vk::AttachmentDescriptionFlags(),
                                                _swapData.format,
                                                vk::SampleCountFlagBits::e1,
                                                vk::AttachmentLoadOp::eClear,
                                                vk::AttachmentStoreOp::eStore,
                                                vk::AttachmentLoadOp::eDontCare,
                                                vk::AttachmentStoreOp::eDontCare,
                                                vk::ImageLayout::eUndefined,
                                                vk::ImageLayout::ePresentSrcKHR);

      vk::AttachmentReference colorAttachmentRef(0,
                                                 vk::ImageLayout::eColorAttachmentOptimal);

      vk::SubpassDescription subpass(vk::SubpassDescriptionFlags(),
                                     vk::PipelineBindPoint::eGraphics,
                                     0,
                                     nullptr,
                                     1,
                                     &colorAttachmentRef,
                                     nullptr,
                                     nullptr,
                                     0,
                                     nullptr);

      vk::SubpassDependency
          subpassDependency(0,
                            VK_SUBPASS_EXTERNAL,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::AccessFlags(),
                            vk::AccessFlagBits::eColorAttachmentWrite,
                            vk::DependencyFlags()
                            );


      vk::RenderPassCreateInfo renderPassCI(vk::RenderPassCreateFlags(),
                                            1,
                                            &colorAttachment,
                                            1,
                                            &subpass,
                                            1,
                                            &subpassDependency);

      _renderPass = _device.createRenderPass(renderPassCI).value;
    }

    void buildCommandBuffers()
    {
      vk::CommandBufferAllocateInfo allocInfo(_commandPool,
                                              vk::CommandBufferLevel::ePrimary,
                                              (uint) _swapData.framebuffers.size());

      // TODO: Fix copy
      _commandBuffers = _device.allocateCommandBuffers(allocInfo).value;

      for (size_t i = 0; i < _commandBuffers.size(); i++)
      {
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(),
                                             nullptr);
        _commandBuffers[i].begin(beginInfo);

        // TODO: Fix weird code for clearValue
        // Currently requires two sub-classes to construct
        const std::array<float, 4> clearColorPrimative = {
          0.5f,
          0.5f,
          0.5f,
          1.0f
        };
        vk::ClearColorValue clearColor(clearColorPrimative);
        const vk::ClearValue clearValue(clearColor);
        vk::RenderPassBeginInfo renderPassInfo(_renderPass,
                                _swapData.framebuffers[i],
                                vk::Rect2D(vk::Offset2D(0, 0),
                                _swapData.extent),
                                1,
                                &clearValue
                                );
        _commandBuffers[i].beginRenderPass(renderPassInfo,
                                           vk::SubpassContents::eInline);
        _commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics,
                                        _graphicsPipeline);

        vk::DeviceSize offsets[] = {0};

        _commandBuffers[i].bindVertexBuffers(0, 1, &_vertexBuffer.localBuffer, offsets);
        _commandBuffers[i].bindIndexBuffer(_indexBuffer.localBuffer, 0, vk::IndexType::eUint16);
        _commandBuffers[i].drawIndexed(6, 1, 0, 0, 0);
        _commandBuffers[i].endRenderPass();
        _commandBuffers[i].end();
      }
    }

    void createTestBuffers(void)
    {
      vk::DeviceSize vertexSize = sizeof(util::testVertices);
      vk::DeviceSize indexSize = sizeof(util::testIndices);

      _vertexBuffer = util::FastBuffer(_device,
                                       _physicalDevice,
                                       _commandPool,
                                       vertexSize,
                                       vk::BufferUsageFlagBits::eVertexBuffer);
      _vertexBuffer.init();
      _vertexBuffer.stage((void *)util::testVertices);
      _vertexBuffer.copy(_graphicsQueue);

      _indexBuffer = util::FastBuffer(_device,
                                      _physicalDevice,
                                      _commandPool,
                                      indexSize,
                                      vk::BufferUsageFlagBits::eIndexBuffer);
      _indexBuffer.init();
      _indexBuffer.stage((void *)util::testIndices);
      _indexBuffer.blockingCopy(_graphicsQueue);
    }
  };
}
#endif // CONTEXT_H
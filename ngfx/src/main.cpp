/*
 * ngfx
 * A 2d rendering library using vulkan with a heavy emphasis on multi viewpoint
 * and headless rendering. It is intended for use with nenbody project
 *
 * The code here is heavily borrowed from these fantastic sources:
 * https://github.com/KhronosGroup/Vulkan-Hpp/tree/master/samples
 * https://github.com/SaschaWillems/Vulkan
 * https://vulkan-tutorial.com/en/
 */

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define VULKAN_HPP_TYPESAFE_CONVERSION
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

// TODO: purge std::vector use
namespace ngfx
{
#if !defined(NDEBUG)
  static bool kDebug = true;
#else
  static bool kDebug = false;
#endif

  static const uint32_t kMaxFramesInFlight = 2;
  static const char * const kValLayers[] = {
    "VK_LAYER_KHRONOS_validation",
  };
  static const uint kValLayerCount = 1;

  static const char * const kDeviceExtensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };
  static const uint kDeviceExtensionCount = 1;

  namespace util
  {
    struct QueueFamilyIndices
    {
      std::optional<uint32_t> graphicsFamily;
      std::optional<uint32_t> presentFamily;

      bool isValid() {
          return graphicsFamily.has_value() && presentFamily.has_value();
      }
    };

    struct SwapchainSupportDetails {
      std::vector<vk::SurfaceFormatKHR> formats;
      std::vector<vk::PresentModeKHR> presentModes;
      vk::SurfaceCapabilitiesKHR capabilites;
      uint32_t padding;
    };

    struct SwapchainData {
      vk::SwapchainKHR swapchain;
      std::vector<vk::Image> images;
      std::vector<vk::ImageView> views;
      std::vector<vk::Framebuffer> framebuffers;
      std::vector<vk::Fence> fences;
      vk::Format format;
      vk::Extent2D extent;
      uint32_t padding;
    };

    struct SemaphoreSet {
      vk::Semaphore imageAvailable;
      vk::Semaphore renderComplete;
    };

    /*
     * Internal utility functions
     */

    static std::vector<const char*> getRequiredExtensions(bool debug)
    {
      // Get required glfw Extensions
      uint32_t glfwExtensionCount = 0;
      const char** glfwExtensions;
      glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

      std::vector<const char*> extensions(glfwExtensions,
                                          glfwExtensions + glfwExtensionCount);

      if (debug)
      {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }

      return extensions;
    }

    static bool checkValidationLayerSupport(void)
    {
      uint32_t layerCount = 0;

      vk::enumerateInstanceLayerProperties (&layerCount,
                                            (vk::LayerProperties *) nullptr);
      std::vector<vk::LayerProperties> availableLayers(layerCount);
      vk::enumerateInstanceLayerProperties (&layerCount,
                                            availableLayers.data());
      for (const char* layerName : kValLayers)
      {
        bool layerFound = false;
        for (const vk::LayerProperties &layer : availableLayers)
        {
          if (strcmp(layer.layerName, layerName) == 0)
          {
            layerFound = true;
            break;
          }
        }
        if (!layerFound)
        {
          return false;
        }
      }
      return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void * pUserData)
    {
      (void) messageSeverity;
      (void) messageType;
      (void) pUserData;

      std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

      return VK_FALSE;
    }

    static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device,
                                                vk::SurfaceKHR surface)
    {
      QueueFamilyIndices indices;
      uint32_t count = 0;

      device.getQueueFamilyProperties(&count, (vk::QueueFamilyProperties *) nullptr);
      std::vector<vk::QueueFamilyProperties> families(count);
      device.getQueueFamilyProperties(&count, families.data());

      uint i = 0;
      for (const vk::QueueFamilyProperties &family : families)
      {
        vk::Bool32 presentSupport = false;
        device.getSurfaceSupportKHR(i, surface, &presentSupport);
        if (presentSupport)
        {
          indices.presentFamily = i;
        }
        if(family.queueFlags & vk::QueueFlagBits::eGraphics)
        {
          indices.graphicsFamily = i;
        }
        if (indices.isValid()) break;
        i++;
      }
      return indices;
    }

    bool checkDeviceExtensionSupport(vk::PhysicalDevice device)
    {
      uint32_t count = 0;
      device.enumerateDeviceExtensionProperties(nullptr,
                                                &count,
                                                (vk::ExtensionProperties *) nullptr);
      std::vector<vk::ExtensionProperties> extensions(count);
      device.enumerateDeviceExtensionProperties(nullptr,
                                                &count,
                                                extensions.data());

      std::set<std::string> requiredExtensions(std::begin(kDeviceExtensions),
                                               std::end(kDeviceExtensions));

      for (const vk::ExtensionProperties& ext : extensions)
      {
        requiredExtensions.erase(ext.extensionName);
      }

      return requiredExtensions.empty();
    }

    SwapchainSupportDetails querySwapchainSupport(vk::PhysicalDevice device,
                                                         vk::SurfaceKHR surface)
    {
      SwapchainSupportDetails details;
      uint32_t formatCount, presentModeCount;

      device.getSurfaceCapabilitiesKHR(surface, &details.capabilites);
      device.getSurfaceFormatsKHR(surface,
                                  &formatCount,
                                  (vk::SurfaceFormatKHR *) nullptr);
      device.getSurfacePresentModesKHR(surface,
                                       &presentModeCount,
                                       (vk::PresentModeKHR *) nullptr);

      if (formatCount != 0)
      {
        details.formats.resize(formatCount);
        device.getSurfaceFormatsKHR(surface, &formatCount, details.formats.data());
      }

      if (presentModeCount !=0)
      {
        details.presentModes.resize(presentModeCount);
        device.getSurfacePresentModesKHR(surface,
                                         &presentModeCount,
                                         details.presentModes.data());
      }
      return details;
    }

    static bool isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface)
    {
      QueueFamilyIndices indices = findQueueFamilies(device, surface);

      bool extensionsSupported = checkDeviceExtensionSupport(device);

      bool swapchainAdequate = false;
      SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device,
                                                                       surface);
      swapchainAdequate = !swapchainSupport.formats.empty()
          && !swapchainSupport.presentModes.empty();

      return (indices.isValid()
              && extensionsSupported
              && swapchainAdequate)
          ? true : false;
    }

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR> & formats)
    {
      for (const vk::SurfaceFormatKHR& f : formats)
      {
        if (f.format == vk::Format::eB8G8R8Srgb
            && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
          return f;
        }
      }
      return formats[0];
    }

    static vk::PresentModeKHR chooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& presentModes)
    {
      for (const vk::PresentModeKHR p : presentModes)
      {
        if (p == vk::PresentModeKHR::eFifoRelaxed)
        {
          return p;
        }
        else if (p == vk::PresentModeKHR::eImmediate)
        {
          return p;
        }
      }
      return vk::PresentModeKHR::eFifo;
    }

    static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                                         uint32_t windowWidth,
                                         uint32_t windowHeight)
    {      
      if (capabilities.currentExtent.width != UINT32_MAX)
      {
        return capabilities.currentExtent;
      }
      else {
        vk::Extent2D actualExtent = {windowWidth, windowHeight};
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width,
                                               actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height,
                                                actualExtent.height));
        return actualExtent;
      }
    }

    /*
     * Public utility Functions
     */

    vk::Instance createInstance(std::string const& appName,
                                std::string const& engineName,
                                uint32_t apiVersion)
    {
      vk::ApplicationInfo applicationInfo(appName.c_str(),
                                          1,
                                          engineName.c_str(),
                                          1,
                                          apiVersion);

      // Get extensions
      std::vector<const char*> ext = getRequiredExtensions(kDebug);

      // Get validation layers
      if(kDebug && !checkValidationLayerSupport())
      {
        throw std::runtime_error("validation layers requested, but not available");
      }

      // Create instance
      vk::InstanceCreateInfo instanceCI(vk::InstanceCreateFlags(),
                                        &applicationInfo,
                                        (kDebug) ? kValLayerCount : 0,
                                        (kDebug) ? kValLayers : nullptr,
                                        static_cast<uint32_t>(ext.size()),
                                        ext.data());
      // Optionally Attach debug messenger
      vk::DebugUtilsMessengerCreateInfoEXT debugCI(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            &debugCallback,
            nullptr);
      instanceCI.setPNext((kDebug) ? &debugCI : nullptr);
      vk::Instance instance = vk::createInstance(instanceCI).value;

      return instance;
    }

    VkResult createDebugMessenger(VkInstance instance,
                                  VkDebugUtilsMessengerEXT* debugMessenger)
    {
      vk::DebugUtilsMessengerCreateInfoEXT debugCI(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debugCallback,
            nullptr);
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance,
            "vkCreateDebugUtilsMessengerEXT");

      return func(instance,
                  (VkDebugUtilsMessengerCreateInfoEXT *) &debugCI,
                  nullptr,
                  debugMessenger);
    }
    void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger)
    {
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance,
            "vkDestroyDebugUtilsMessengerEXT");
      return func(instance, debugMessenger, nullptr);
    }

    vk::PhysicalDevice pickPhysicalDevice(vk::Instance instance,
                                          vk::SurfaceKHR surface)
    {
      uint32_t count = 0;
      instance.enumeratePhysicalDevices(&count, (vk::PhysicalDevice *) nullptr);

      if (count == 0)
      {
        throw std::runtime_error("failed to find GPUs with Vulkan Support");
      }

      std::vector<vk::PhysicalDevice> devices(count);
      instance.enumeratePhysicalDevices(&count, devices.data());

      vk::PhysicalDevice selectedDevice;
      for (const vk::PhysicalDevice &device : devices)
      {
        if(isDeviceSuitable(device, surface))
        {
          selectedDevice = device;
        }
      }

      if (selectedDevice == vk::PhysicalDevice())
      {
        throw std::runtime_error("failed to find a suitable GPU");
      } return selectedDevice;
    }

    vk::Device createLogicalDevice(vk::PhysicalDevice physicalDevice,
                                   QueueFamilyIndices indices)
    {
      float priority = 1.0f;
      std::vector<vk::DeviceQueueCreateInfo> queuesCI;
      // Note: set will only contain a single value if present=graphics queue
      std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
      };

      for (uint32_t q : uniqueQueueFamilies)
      {
        vk::DeviceQueueCreateInfo qCI(vk::DeviceQueueCreateFlags(),
                                      q,
                                      1,
                                      &priority);
        queuesCI.push_back(qCI);
      }

      vk::PhysicalDeviceFeatures features;

      vk::DeviceCreateInfo deviceCI(vk::DeviceCreateFlags(),
                                    (uint) queuesCI.size(),
                                    queuesCI.data(),
                                    kValLayerCount,
                                    kValLayers,
                                    kDeviceExtensionCount,
                                    kDeviceExtensions,
                                    &features
                                    );     
      return physicalDevice.createDevice(deviceCI).value;
    }

    // TODO: Fix messy swapdata initialization
    void createSwapchain(SwapchainData *swapData,
                         QueueFamilyIndices qFamilies,
                         vk::Device device,
                         vk::SurfaceKHR surface,
                         SwapchainSupportDetails info,
                         uint32_t width,
                         uint32_t height)
    {
      vk::SurfaceFormatKHR format = chooseSwapSurfaceFormat(info.formats);
      vk::PresentModeKHR presentMode = chooseSwapPresentMode(info.presentModes);

      vk::Extent2D extent(width, height);

      // Save swapchain data
      swapData->format = format.format;
      swapData->extent = extent;

      uint32_t imageCount = info.capabilites.minImageCount + 1;
      if (info.capabilites.maxImageCount > 0
          && imageCount > info.capabilites.maxImageCount)
      {
        imageCount = info.capabilites.maxImageCount;
      }

      uint32_t indices[] = {
        qFamilies.graphicsFamily.value(),
        qFamilies.presentFamily.value()
      };
      bool unifiedQ = (qFamilies.graphicsFamily.value()
                           == qFamilies.presentFamily.value());

      vk::SwapchainCreateInfoKHR swapchainCI(vk::SwapchainCreateFlagsKHR(),
                                             surface,
                                             imageCount,
                                             format.format,
                                             format.colorSpace,
                                             extent,
                                             1,
                                             vk::ImageUsageFlagBits::eColorAttachment,
                                             (unifiedQ) ? vk::SharingMode::eExclusive
                                                        : vk::SharingMode::eConcurrent,
                                             (unifiedQ) ? 0
                                                        : 2,
                                             (unifiedQ) ? nullptr
                                                        : indices,
                                             info.capabilites.currentTransform,
                                             vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                             presentMode,
                                             VK_TRUE, // clipped
                                             vk::SwapchainKHR()); // old swapchain
      // Create swapchain
      swapData->swapchain = device.createSwapchainKHR(swapchainCI).value;
      uint32_t swapchainImageCount = 0;
      device.getSwapchainImagesKHR(swapData->swapchain,
                                   &swapchainImageCount,
                                   (vk::Image *) nullptr);
      swapData->images.resize(swapchainImageCount);

      // Get images
      device.getSwapchainImagesKHR(swapData->swapchain,
                                   &swapchainImageCount,
                                   swapData->images.data());

      // Create imageviews
      swapData->views.resize(swapchainImageCount);
      for (size_t i = 0; i < swapchainImageCount; i++)
      {
        vk::ImageViewCreateInfo viewCI(vk::ImageViewCreateFlags(),
                                       swapData->images[i],
                                       vk::ImageViewType::e2D,
                                       swapData->format,
                                       vk::ComponentMapping(),
                                       vk::ImageSubresourceRange(
                                         vk::ImageAspectFlagBits::eColor,
                                         0, //baseMipLevel
                                         1, //levelCount
                                         0, //baseArrayLayer
                                         1) //layerCount
                                       );

        swapData->views[i] = device.createImageView(viewCI).value;
      }
    }

    static std::vector<char> readFile(const std::string& filename)
    {
      std::ifstream file(filename, std::ios::ate | std::ios::binary);

      if (!file.is_open())
      {
        throw std::runtime_error("failed to open file");
      }

      size_t fileSize = (size_t) file.tellg();
      std::vector<char> buffer(fileSize);

      file.seekg(0);
      file.read(buffer.data(), (long) fileSize);

      file.close();

      return buffer;
    }

    vk::ShaderModule createShaderModule(vk::Device device,
                                        const std::vector<char>& code)
    {
      vk::ShaderModuleCreateInfo shaderCI(vk::ShaderModuleCreateFlags(),
                                          code.size(),
                                          reinterpret_cast<const uint32_t*>(
                                            code.data()));

      return device.createShaderModule(shaderCI).value;
    }

    void createFramebuffers(SwapchainData *swapData,
                            vk::Device device,
                            vk::RenderPass renderPass)
    {
      swapData->framebuffers.resize(swapData->views.size());
      for(size_t i = 0; i < swapData->views.size(); i++)
      {
        vk::ImageView attachments[] =
        {
          swapData->views[i]
        };

        vk::FramebufferCreateInfo framebufferCI(vk::FramebufferCreateFlags(),
                                                renderPass,
                                                1,
                                                attachments,
                                                swapData->extent.width,
                                                swapData->extent.height,
                                                1);

        swapData->framebuffers[i] = device.createFramebuffer(framebufferCI).value;
      }
    }

    vk::CommandPool createCommandPool(vk::Device device,
                                      QueueFamilyIndices indices)
    {
      vk::CommandPoolCreateInfo poolCI(vk::CommandPoolCreateFlags(),
                                       indices.graphicsFamily.value());

      return device.createCommandPool(poolCI).value;
    }
  }

  // TODO: Docs
  // Main class for setting up and running ngfx
  class Context
  {
  public:
    static const uint32_t kWidth = 800;
    static const uint32_t kHeight = 600;
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

    util::SemaphoreSet _semaphores[kMaxFramesInFlight];
    vk::Fence _inFlightFences[kMaxFramesInFlight];
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

      buildCommandBuffers();

      for (uint i = 0; i < kMaxFramesInFlight; i++)
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
          printf("%f ms/frame: %f fps\n", 1000.0/double(nbFrames));
          nbFrames = 0;
          lastTime += 1.0;
        }
        drawFrame();
      }
      _device.waitIdle();
    }

    void cleanup()
    {
      cleanupSwapchain();
      for (uint i = 0; i < kMaxFramesInFlight; i++)
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
      vk::Result acquireResult = _device.acquireNextImageKHR(_swapData.swapchain,
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

      vk::Result presentResult = _presentQueue.presentKHR(&presentInfo);
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

      vk::PipelineVertexInputStateCreateInfo
          vertexInputCI(vk::PipelineVertexInputStateCreateFlags(),
                        0,
                        nullptr,
                        0,
                        nullptr);

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

        _commandBuffers[i].draw(3, 1, 0, 0);

        _commandBuffers[i].endRenderPass();
        _commandBuffers[i].end();
      }
    }
  };
}


int main() {
  ngfx::Context app;

  try {
    app.init();
    app.renderTest();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

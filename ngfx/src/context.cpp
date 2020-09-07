// Source for context struct 
#include "ngfx.hpp"
#include "util.hpp"

namespace ngfx
{
  Context::Context()
  {
    // TODO: Move window creation out of Context
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, kResizable);
    window = glfwCreateWindow(
        kWidth,
        kHeight,
        "ngfx",
        nullptr,
        nullptr);
    
    glfwSetWindowUserPointer(window, this); 
   
    util::createInstance("test", "ngfx", VK_API_VERSION_1_1, &instance);
    util::createDebugMessenger((VkInstance *) &instance,
                               (VkDebugUtilsMessengerEXT *) &debugMessenger);
    
    glfwCreateWindowSurface(
        instance,
        window,
        nullptr,
        (VkSurfaceKHR *) &surface);

    physicalDevice = util::pickPhysicalDevice(&instance, &surface); 
    msaaSamples = util::getMaxUsableSampleCount(&physicalDevice);

    util::findQueueFamilies(&physicalDevice, &surface, &qFamilies);
    util::createLogicalDevice(&physicalDevice, &qFamilies, &device);
    graphicsQueue = device.getQueue(qFamilies.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(qFamilies.presentFamily.value(), 0);
    transferQueue = device.getQueue(qFamilies.transferFamily.value(), 0);

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    vmaCreateAllocator(&allocatorInfo, &allocator);
    
    util::querySwapchainSupport(&physicalDevice, &surface, &swapInfo);

    // Defining a single pipelineCache to be shared for the whole context
    // According to Vendors (Nvidia do's & dont's) this is recommended 
    vk::PipelineCacheCreateInfo cacheCI(
        vk::PipelineCacheCreateFlags(),
        0,
        nullptr);
    device.createPipelineCache(&cacheCI, nullptr, &pipelineCache);

    // command pool
    // TODO: multiple pools for threaded recording
    cmdPool = util::createCommandPool(&device,  qFamilies);
  };

  // TODO: Add destructor to clean up vk objs
  // Currently relies on cleanup() in test class which is bad 
  Context::~Context()
  {
    device.destroyCommandPool(cmdPool);
    device.destroy();
    util::DestroyDebugUtilsMessengerEXT(instance, debugMessenger);
    instance.destroySurfaceKHR(surface);
    instance.destroy();
    
    glfwDestroyWindow(window);
    glfwTerminate();
  };
}


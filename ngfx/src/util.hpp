#ifndef UTIL_H
#define UTIL_H

#include "ngfx.hpp"
#include "config.hpp"

namespace ngfx
{
  namespace util
  {

    uint32_t findMemoryType(vk::PhysicalDevice &phys,
                            uint32_t typeFilter,
                            vk::MemoryPropertyFlags requiredProps);

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

    //TODO: Refactor along with vertex binding and attr code to make this useful
    struct Vertex {
      glm::vec2 pos;
      glm::vec3 color;

      static vk::VertexInputBindingDescription getBindingDescription()
      {
        vk::VertexInputBindingDescription
            bindingDescription(0,
                               sizeof(util::Vertex),
                               vk::VertexInputRate::eVertex);
        return bindingDescription;
      }
    };

    //TODO: Refactor along with vertex binding and attr code to make this useful
    struct Instance {
      glm::vec2 pos;

      static vk::VertexInputBindingDescription getBindingDescription(void)
      {
        vk::VertexInputBindingDescription
            bindingDescription(1,
                               sizeof(util::Instance),
                               vk::VertexInputRate::eInstance);
        return bindingDescription;
      }
    };

    struct Mvp {
      glm::mat4 model;
      glm::mat4 view;
      glm::mat4 proj;
    };

    // Abstracts buffer and transfer semantics for a fast uniform/vertex buffer
    // that is easy to work with on the CPU side
    // TODO: batch buffer allocations
    struct FastBuffer
    {
    public:
      bool valid;
      vk::Device *device;
      vk::PhysicalDevice *phys;
      vk::CommandPool *pool;
      vk::DeviceSize size;
      vk::BufferUsageFlags usage;
      vk::Buffer stagingBuffer;
      vk::DeviceMemory stagingMemory;
      vk::Buffer localBuffer;
      vk::DeviceMemory localMemory;
      vk::CommandBuffer commandBuffer;

      FastBuffer(void) : valid(false) {};

      FastBuffer(vk::Device &dev,
                 vk::PhysicalDevice &phys,
                 vk::CommandPool &pool,
                 vk::DeviceSize size,
                 vk::BufferUsageFlags usage)
        : valid(false), device(&dev), phys(&phys), pool(&pool), size(size),
          usage(usage)
      {}

      void init(void)
      {
        vk::BufferCreateInfo stagingCI(vk::BufferCreateFlagBits(),
                                      size,
                                      vk::BufferUsageFlagBits::eTransferSrc,
                                      // TODO: support concurrent
                                      vk::SharingMode::eExclusive,
                                      0,
                                      nullptr);
        device->createBuffer(&stagingCI,
                             nullptr,
                             &stagingBuffer);

        vk::BufferCreateInfo localCI(vk::BufferCreateFlagBits(),
                                      size,
                                      vk::BufferUsageFlagBits::eTransferDst
                                      | usage,
                                      // TODO: support concurrent
                                      vk::SharingMode::eExclusive,
                                      0,
                                      nullptr);
        device->createBuffer(&localCI,
                             nullptr,
                             &localBuffer);

        vk::MemoryRequirements stagingReqs =
            device->getBufferMemoryRequirements(stagingBuffer);
        vk::MemoryRequirements localReqs =
            device->getBufferMemoryRequirements(localBuffer);

        uint32_t stagingIndex =
            util::findMemoryType(*phys,
                                 stagingReqs.memoryTypeBits,
                                 vk::MemoryPropertyFlagBits::eHostVisible
                                 | vk::MemoryPropertyFlagBits::eHostCoherent
                                 | vk::MemoryPropertyFlagBits::eHostCached);
        uint32_t localIndex =
            util::findMemoryType(*phys,
                                 localReqs.memoryTypeBits,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal);

        vk::MemoryAllocateInfo stagingAllocInfo(stagingReqs.size, stagingIndex);
        device->allocateMemory(&stagingAllocInfo, nullptr, &stagingMemory);
        device->bindBufferMemory(stagingBuffer, stagingMemory, 0);
        vk::MemoryAllocateInfo localAllocInfo(localReqs.size, localIndex);
        device->allocateMemory(&stagingAllocInfo, nullptr, &localMemory);
        device->bindBufferMemory(localBuffer, localMemory, 0);

        // Create and record transfer command buffer
        vk::CommandBufferAllocateInfo allocInfo(*pool,
                                                vk::CommandBufferLevel::ePrimary,
                                                1);
        commandBuffer = device->allocateCommandBuffers(allocInfo).value.front();

        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(),
                                             nullptr);
        commandBuffer.begin(beginInfo);
        vk::BufferCopy copyRegion(0, 0, size);
        commandBuffer.copyBuffer(stagingBuffer, localBuffer, 1, (const vk::BufferCopy *) &copyRegion);
        commandBuffer.end();

        // Map memory
        device->mapMemory(stagingMemory, 0, size, vk::MemoryMapFlags(), &_handle);
        valid = true;
      }

      void stage(void* data)
      {
        assert(valid);
        memcpy(_handle, data, size);
      }

      // TODO: add fences for external sync
      void copy(vk::Queue q)
      {
        assert(valid);
        vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);
        q.submit(1, &submitInfo, vk::Fence());
      }

      void blockingCopy(vk::Queue q)
      {
        assert(valid);
        vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);
        q.submit(1, &submitInfo, vk::Fence());
        q.waitIdle();
      }

      ~FastBuffer(void)
      {
        if (valid)
        {
          device->unmapMemory(stagingMemory);
          device->freeCommandBuffers(*pool, 1, (const vk::CommandBuffer *)&commandBuffer);
          device->freeMemory(stagingMemory);
          device->freeMemory(localMemory);
          device->destroyBuffer(stagingBuffer);
          device->destroyBuffer(localBuffer);
        }
      }
    private:
      void *_handle;
    };

    struct Fbo
    {
    public:
      vk::Image image;
      vk::ImageView view;
      vk::DeviceMemory mem;
      vk::Framebuffer frame;
    };

    const Vertex testVertices[] = {
      {{-0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}},
      {{0.0f, 0.5f}, {0.9f, 0.9f, 0.9f}},
      {{0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}},
    };

    const uint16_t testIndices[] = {
      0, 1, 2, 0
    };

    const Instance testInstances[] = {
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
      for (const char* layerName : ngfx::kValLayers)
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

    static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice phys,
                                                vk::SurfaceKHR surface)
    {
      QueueFamilyIndices indices;
      uint32_t count = 0;

      phys.getQueueFamilyProperties(&count, (vk::QueueFamilyProperties *) nullptr);
      std::vector<vk::QueueFamilyProperties> families(count);
      phys.getQueueFamilyProperties(&count, families.data());

      uint i = 0;
      for (const vk::QueueFamilyProperties &family : families)
      {
        vk::Bool32 presentSupport = false;
        phys.getSurfaceSupportKHR(i, surface, &presentSupport);
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

    bool checkDeviceExtensionSupport(vk::PhysicalDevice phys)
    {
      uint32_t count = 0;
      phys.enumerateDeviceExtensionProperties(nullptr,
                                                &count,
                                                (vk::ExtensionProperties *) nullptr);
      std::vector<vk::ExtensionProperties> extensions(count);
      phys.enumerateDeviceExtensionProperties(nullptr,
                                                &count,
                                                extensions.data());

      std::set<std::string> requiredExtensions(std::begin(ngfx::kDeviceExtensions),
                                               std::end(ngfx::kDeviceExtensions));

      for (vk::ExtensionProperties &ext : extensions)
      {
        requiredExtensions.erase((std::string) ext.extensionName);
      }

      return requiredExtensions.empty();
    }

    SwapchainSupportDetails querySwapchainSupport(vk::PhysicalDevice phys,
                                                         vk::SurfaceKHR surface)
    {
      SwapchainSupportDetails details;
      uint32_t formatCount, presentModeCount;

      phys.getSurfaceCapabilitiesKHR(surface, &details.capabilites);
      phys.getSurfaceFormatsKHR(surface,
                                  &formatCount,
                                  (vk::SurfaceFormatKHR *) nullptr);
      phys.getSurfacePresentModesKHR(surface,
                                       &presentModeCount,
                                       (vk::PresentModeKHR *) nullptr);

      if (formatCount != 0)
      {
        details.formats.resize(formatCount);
        phys.getSurfaceFormatsKHR(surface, &formatCount, details.formats.data());
      }

      if (presentModeCount !=0)
      {
        details.presentModes.resize(presentModeCount);
        phys.getSurfacePresentModesKHR(surface,
                                         &presentModeCount,
                                         details.presentModes.data());
      }
      return details;
    }

    static bool isDeviceSuitable(vk::PhysicalDevice phys, vk::SurfaceKHR surface)
    {
      QueueFamilyIndices indices = findQueueFamilies(phys, surface);

      bool extensionsSupported = checkDeviceExtensionSupport(phys);

      bool swapchainAdequate = false;
      SwapchainSupportDetails swapchainSupport = querySwapchainSupport(phys,
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
      std::vector<const char*> ext = getRequiredExtensions(ngfx::kDebug);

      // Get validation layers
      if(ngfx::kDebug && !checkValidationLayerSupport())
      {
        throw std::runtime_error("validation layers requested, but not available");
      }

      // Create instance
      vk::InstanceCreateInfo instanceCI(vk::InstanceCreateFlags(),
                                        &applicationInfo,
                                        (ngfx::kDebug) ? ngfx::kValLayerCount : 0,
                                        (ngfx::kDebug) ? ngfx::kValLayers : nullptr,
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
      instanceCI.setPNext((ngfx::kDebug) ? &debugCI : nullptr);
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
                                    ngfx::kValLayerCount,
                                    ngfx::kValLayers,
                                    ngfx::kDeviceExtensionCount,
                                    ngfx::kDeviceExtensions,
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

    uint32_t findMemoryType(vk::PhysicalDevice &phys,
                            uint32_t typeFilter,
                            vk::MemoryPropertyFlags requiredProps)
    {
      vk::PhysicalDeviceMemoryProperties memProps =
          phys.getMemoryProperties();

      for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
      {
        if ((typeFilter & (1 << i)
            && (memProps.memoryTypes[i].propertyFlags & requiredProps) == requiredProps))
        {
          return i;
        }
      }
      throw std::runtime_error("failed to find suitable memory type!");
    }
  }
}
#endif // UTIL_H

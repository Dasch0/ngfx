
#include "util.hpp"
#include "ngfx.hpp"
#include "config.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace ngfx
{
  namespace util
  {
    uint32_t findMemoryType(vk::PhysicalDevice &phys,
                            uint32_t typeFilter,
                            vk::MemoryPropertyFlags requiredProps);

    bool QueueFamilyIndices::isValid()
    {
      return graphicsFamily.has_value() 
        && presentFamily.has_value()
        && transferFamily.has_value();
    }

    std::vector<const char*> getRequiredExtensions(bool debug)
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

    bool checkValidationLayerSupport(void)
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

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void * pUserData)
    {
      (void) messageSeverity;
      (void) messageType;
      (void) pUserData;

      if (messageSeverity
          & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
      {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
      }
      return VK_FALSE;
    }

    // TODO: maybe move this into QueueFamilyIndices?
    void findQueueFamilies(vk::PhysicalDevice *phys,
                      vk::SurfaceKHR *surface,
                      QueueFamilyIndices *qFamilies)
    {
      uint32_t count = 0;

      phys->getQueueFamilyProperties(&count,
                                    (vk::QueueFamilyProperties *) nullptr);
      std::vector<vk::QueueFamilyProperties> families(count);
      phys->getQueueFamilyProperties(&count, families.data());

      uint i = 0;
      for (const vk::QueueFamilyProperties &family : families)
      {
        vk::Bool32 presentSupport = false;
        phys->getSurfaceSupportKHR(i, *surface, &presentSupport);
        if (presentSupport)
        {
          qFamilies->presentFamily = i;
        }
        if(family.queueFlags & vk::QueueFlagBits::eGraphics)
        {
          qFamilies->graphicsFamily = i;
        }
        if((family.queueFlags & vk::QueueFlagBits::eTransfer)
           && !(family.queueFlags & vk::QueueFlagBits::eGraphics))
        {
          qFamilies->transferFamily = i;
        }
        if (qFamilies->isValid()) break;
        i++;
      }

      // If a dedicated transfer queue is not found, use graphics
      if( !qFamilies->transferFamily.has_value())
      {
        qFamilies->transferFamily = qFamilies->graphicsFamily;
      }
    }

    bool checkDeviceExtensionSupport(vk::PhysicalDevice *phys)
    {
      uint32_t count = 0;
      phys->enumerateDeviceExtensionProperties(nullptr,
                                                &count,
                                                (vk::ExtensionProperties *) nullptr);
      std::vector<vk::ExtensionProperties> extensions(count);
      phys->enumerateDeviceExtensionProperties(nullptr,
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

    // TODO: maybe move this into SwapchainSupportDetails
    void querySwapchainSupport(vk::PhysicalDevice *phys,
                               vk::SurfaceKHR *surface,
                               SwapchainSupportDetails *details)
    {

      uint32_t formatCount, presentModeCount;

      phys->getSurfaceCapabilitiesKHR(*surface, &(details->capabilites));
      phys->getSurfaceFormatsKHR(*surface,
                                 &formatCount,
                                 (vk::SurfaceFormatKHR *) nullptr);
      phys->getSurfacePresentModesKHR(*surface,
                                     &presentModeCount,
                                     (vk::PresentModeKHR *) nullptr);

      if (formatCount != 0)
      {
        details->formats.resize(formatCount);
        phys->getSurfaceFormatsKHR(*surface, &formatCount, details->formats.data());
      }

      if (presentModeCount !=0)
      {
        details->presentModes.resize(presentModeCount);
        phys->getSurfacePresentModesKHR(*surface,
                                        &presentModeCount,
                                        details->presentModes.data());
      }
    }

    bool isDeviceSuitable(vk::PhysicalDevice *phys,
                                 vk::SurfaceKHR *surface)
    {
      bool extensionsSupported = checkDeviceExtensionSupport(phys);

      bool swapchainAdequate = false;
      
      SwapchainSupportDetails details;
      querySwapchainSupport(phys,
                            surface,
                            &details);
      swapchainAdequate = !details.formats.empty()
          && !details.presentModes.empty();

      return (extensionsSupported
              && swapchainAdequate)
          ? true : false;
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
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

    vk::PresentModeKHR chooseSwapPresentMode(
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

   vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
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

    void createInstance(std::string const& appName,
                        std::string const& engineName,
                        uint32_t apiVersion,
                        vk::Instance *instance)
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
      
      vk::createInstance(&instanceCI, nullptr, instance); 
    }

    VkResult createDebugMessenger(VkInstance *instance,
                                  VkDebugUtilsMessengerEXT* debugMessenger)
    {
      vk::DebugUtilsMessengerCreateInfoEXT debugCI(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debugCallback,
            nullptr);
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            *instance,
            "vkCreateDebugUtilsMessengerEXT");

      return func(*instance,
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

    vk::PhysicalDevice pickPhysicalDevice(vk::Instance *instance,
                                          vk::SurfaceKHR *surface)
    {
      uint32_t count = 0;
      instance->enumeratePhysicalDevices(&count, (vk::PhysicalDevice *) nullptr);

      if (count == 0)
      {
        throw std::runtime_error("failed to find GPUs with Vulkan Support");
      }

      std::vector<vk::PhysicalDevice> devices(count);
      instance->enumeratePhysicalDevices(&count, devices.data());

      vk::PhysicalDevice selectedDevice;
      for (int i = 0; i < devices.size(); i++)
      {
        if(isDeviceSuitable(&devices[i], surface))
        {
          selectedDevice = devices[i];
        }
      }

      if (selectedDevice == vk::PhysicalDevice())
      {
        throw std::runtime_error("failed to find a suitable GPU");
      }
      return selectedDevice;
    }

    void createLogicalDevice(vk::PhysicalDevice *physicalDevice,
                                   QueueFamilyIndices *indices,
                                   vk::Device *device)
    {
      float priority = 1.0f;
      std::vector<vk::DeviceQueueCreateInfo> queuesCI;
      // Note: set will alias identical queueFamilies
      // e.g if present=graphics family
      std::set<uint32_t> uniqueQueueFamilies = {
        indices->graphicsFamily.value(),
        indices->presentFamily.value(),
        indices->transferFamily.value()
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
      physicalDevice->createDevice(&deviceCI, nullptr, device);
    }

    std::vector<char> readFile(const std::string& filename)
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

    vk::ShaderModule createShaderModule(vk::Device *device,
                                        const std::vector<char>& code)
    {
      vk::ShaderModuleCreateInfo shaderCI(vk::ShaderModuleCreateFlags(),
                                          code.size(),
                                          reinterpret_cast<const uint32_t*>(
                                            code.data()));
      vk::ShaderModule shader;
      device->createShaderModule(&shaderCI, nullptr, &shader);
      return shader;
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
            && (memProps.memoryTypes[i].propertyFlags
                & requiredProps) == requiredProps))
        {
          return i;
        }
      }
      throw std::runtime_error("failed to find suitable memory type!");
    }
    
    vk::SampleCountFlags getMaxUsableSampleCount(vk::PhysicalDevice *d) {
      vk::PhysicalDeviceProperties prop = d->getProperties();

      vk::SampleCountFlags counts =
        prop.limits.framebufferColorSampleCounts 
        & prop.limits.framebufferDepthSampleCounts;

      return vk::SampleCountFlags(counts);
    }
  }
}

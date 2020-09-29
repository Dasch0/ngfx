#ifndef UTIL_H
#define UTIL_H

#include "config.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace ngfx
{
  namespace util
  {
    template <typename T, size_t N>
      constexpr size_t array_size(T (&)[N]) {
            return N;
      }

    struct QueueFamilyIndices
    {
      std::optional<uint32_t> graphicsFamily;
      std::optional<uint32_t> presentFamily;
      std::optional<uint32_t> transferFamily;

      bool isValid();
    };

    struct SwapchainSupportDetails {
      std::vector<vk::SurfaceFormatKHR> formats;
      std::vector<vk::PresentModeKHR> presentModes;
      vk::SurfaceCapabilitiesKHR capabilites;
      uint32_t padding;
    };

    struct SemaphoreSet {
      vk::Semaphore imageAvailable;
      vk::Semaphore renderComplete;
    };

    struct Fbo
    {
      vk::Image image;
      vk::ImageView view;
      vk::Extent2D extent;
      vk::DeviceMemory mem;
      vk::Framebuffer frame;
    };

    std::vector<const char*> getRequiredExtensions(bool debug);
    
    bool checkValidationLayerSupport(void);
    
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void * pUserData);
  
    // TODO: maybe move this into QueueFamilyIndices      
    void findQueueFamilies(
        vk::PhysicalDevice *phys,
        vk::SurfaceKHR *surface,
        QueueFamilyIndices *qFamilies);
    
    bool checkDeviceExtensionSupport(vk::PhysicalDevice *phys);
   
    // TODO: maybe move this into SwapchainSupportDetails
    void querySwapchainSupport(
        vk::PhysicalDevice *phys,
        vk::SurfaceKHR *surface,
        SwapchainSupportDetails *details);
    
    bool isDeviceSuitable(
        vk::PhysicalDevice *phys,
        vk::SurfaceKHR *surface);

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR> & formats);
   
    vk::PresentModeKHR chooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& presentModes);
   
    vk::Extent2D chooseSwapExtent(
        const vk::SurfaceCapabilitiesKHR& capabilities,
        uint32_t windowWidth,
        uint32_t windowHeight);
    
    void createInstance(
        std::string const& appName,
        std::string const& engineName,
        uint32_t apiVersion,
        vk::Instance *instance);
    
    VkResult createDebugMessenger(
        VkInstance *instance,
        VkDebugUtilsMessengerEXT* debugMessenger);
  
    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger);
   
    vk::PhysicalDevice pickPhysicalDevice(
        vk::Instance *instance,
        vk::SurfaceKHR *surface);
   
    void createLogicalDevice(
        vk::PhysicalDevice *physicalDevice,
        QueueFamilyIndices *indices,
        vk::Device *device);
   
    std::vector<char> readFile(const std::string& filename);
    
    // TODO: Fix copy constructor use, add shader module pointer as arg
    vk::ShaderModule createShaderModule(
        vk::Device *device,
        const std::vector<char>& code);
    
    // TODO: Fix copy constructor use, add pool pointer as arg
    vk::CommandPool createCommandPool(
        vk::Device *device,
        QueueFamilyIndices indices);
    
    uint32_t findMemoryType(
        vk::PhysicalDevice &phys,
        uint32_t typeFilter,
        vk::MemoryPropertyFlags requiredProps);

    vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice *d);
  }
}
#endif // UTIL_H

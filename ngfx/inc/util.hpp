#ifndef UTIL_H
#define UTIL_H

#include "ngfx.hpp"
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

    //TODO: Refactor along with vertex binding and attr code to make this useful
    //TODO: AOSOA layout
    struct Vertex {
      glm::vec2 pos;
      glm::vec3 color;
      glm::vec2 texCoord;

      static vk::VertexInputBindingDescription getBindingDescription();
    };

    //TODO: Refactor along with vertex binding and attr code to make this useful
    struct Instance {
      glm::vec2 pos;

      static vk::VertexInputBindingDescription getBindingDescription(void);
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

      FastBuffer(void);

      FastBuffer(vk::Device &dev,
                 vk::PhysicalDevice &phys,
                 vk::CommandPool &pool,
                 vk::DeviceSize size,
                 vk::BufferUsageFlags usage);

      void init(void);
      void stage(void* data);
      void copy(vk::Queue q);
      void blockingCopy(vk::Queue q);
      ~FastBuffer(void);
    
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
      {{-0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}, {0.0f, 0.0f}},
      {{0.0f, 0.5f}, {0.9f, 0.9f, 0.9f}, {1.0f, 0.0f}},
      {{0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}, {1.0f, 1.0f}},
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

    std::vector<const char*> getRequiredExtensions(bool debug);
    
    bool checkValidationLayerSupport(void);
    
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void * pUserData);
  
    // TODO: maybe move this into QueueFamilyIndices      
    void findQueueFamilies(vk::PhysicalDevice *phys,
                           vk::SurfaceKHR *surface,
                           QueueFamilyIndices *qFamilies);
    
    bool checkDeviceExtensionSupport(vk::PhysicalDevice *phys);
   
    // TODO: maybe move this into SwapchainSupportDetails
    void querySwapchainSupport(vk::PhysicalDevice *phys,
                               vk::SurfaceKHR *surface,
                               SwapchainSupportDetails *details);
    
    bool isDeviceSuitable(vk::PhysicalDevice *phys,
                          vk::SurfaceKHR *surface);

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR> & formats);
   
    vk::PresentModeKHR chooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& presentModes);
   
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                                  uint32_t windowWidth,
                                  uint32_t windowHeight);
    
    void createInstance(std::string const& appName,
                                std::string const& engineName,
                                uint32_t apiVersion,
                                vk::Instance *instance);
    
    VkResult createDebugMessenger(VkInstance *instance,
                                  VkDebugUtilsMessengerEXT* debugMessenger);
  
    void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger);
   
    vk::PhysicalDevice pickPhysicalDevice(vk::Instance *instance,
                                          vk::SurfaceKHR *surface);
   
    void createLogicalDevice(vk::PhysicalDevice *physicalDevice,
                                   QueueFamilyIndices *indices,
                                   vk::Device *device);
   
    std::vector<char> readFile(const std::string& filename);
    
    // TODO: Fix copy constructor use, add shader module pointer as arg
    vk::ShaderModule createShaderModule(vk::Device device,
                                        const std::vector<char>& code);
    
    // TODO: Fix copy constructor use, add pool pointer as arg
    vk::CommandPool createCommandPool(vk::Device device,
                                      QueueFamilyIndices indices);
    
    uint32_t findMemoryType(vk::PhysicalDevice &phys,
                            uint32_t typeFilter,
                            vk::MemoryPropertyFlags requiredProps);
  }
}
#endif // UTIL_H

#ifndef CONTEXT_H
#define CONTEXT_H

#include <vulkan/vulkan.hpp>
#include "ngfx.hpp"
#include "config.hpp"
#include "util.hpp"

// TODO: Docs
namespace ngfx
{
 struct Context
  {
    // TODO: implement resizability & vsync
    static const uint32_t kWidth = 800;
    static const uint32_t kHeight = 600;
    static const bool kResizable = false;
    static const bool kVsync = false;

    GLFWwindow *window;
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::SurfaceKHR surface;

    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    util::QueueFamilyIndices qFamilies;
    vk::Queue presentQueue;
    vk::Queue graphicsQueue;
    vk::Queue transferQueue;
    util::SwapchainSupportDetails swapInfo;
    vk::PipelineCache pipelineCache;

    Context();
    ~Context();
  };  
}
#endif // CONTEXT_H

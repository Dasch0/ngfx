#ifndef CONTEXT_H
#define CONTEXT_H

#include <vulkan/vulkan.hpp>
#include "GLFW/glfw3.h"
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
    // TODO: move window outside of context
    // allow for support of other window managers
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
    vk::CommandPool cmdPool;

    // Useful configuration info
    vk::SampleCountFlags msaaSamples;
    Context();
    ~Context();
  };  
}
#endif // CONTEXT_H

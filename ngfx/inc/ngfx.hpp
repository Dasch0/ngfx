#ifndef NGFX_H
#define NGFX_H

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
#include <cstring>
#include <optional>
#include <set>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#define VULKAN_HPP_TYPESAFE_CONVERSION
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>
#include "util.hpp"

namespace ngfx {

  /* Basic memory arena implementation for storing renderer data
   * TODO: expand this, maybe move to external library
   */
  static const size_t kArenaSize = 536870912;
  static const int8_t kAlignment = 16; // alignment in bytes
  alignas(128) static uint8_t arena[kArenaSize];
  static size_t used = 0; // will always be kAlignment aligned
  
  /*
   * Handle
   *  A basic type used for accessing any table or list of data
   */
  template<typename T>
  struct Handle {
    T *ptr;
    int32_t cnt;

    Handle()
      : ptr(NULL), cnt(-1)
    {}

    Handle(T *pointer, int32_t count)
      : ptr(pointer), cnt(count) 
    {}

    T &operator[](int32_t i)
    {
      return ptr[i];
    }
  };

  /* alloc
   *  Allocates new data in the memory arena, returns a Handle
   *  to the created array.
   */
  template<typename T>
  inline Handle<T> alloc(int32_t cnt)
  {
    T *ptr = (T *)(arena + used); // assume used is aligned
    size_t s = sizeof(T) * cnt;
 
    // Ensure used is aligned for next alloc
    auto mod = (ptr + s) & (kAlignment-1);
    used += s + (mod != 0) * (kAlignment - mod);
    assert(used < kArenaSize);
    memset(ptr, 0, s);
    return Handle<T>(ptr, cnt);
  }

  /* 
   * Vertex
   *  Defines vertex format and layout for GPU side
   *  See shaders/, ngfx::attributeLayout, and ngfx::bindingLayout
   *  for dependencies on this format
   */
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;   
  };

  typedef uint16_t Index;

  /* Instance
   *  Storage of the model transforms used when rendering multiple 
   *  instances of a mesh
   */
  struct Instance {
    glm::mat4 pos;
  };

  /*
   * attributeLayout
   *  Defines the attribute layout of the vertex and index data used by all
   *  pipelines
   */
  static constexpr vk::VertexInputAttributeDescription attributeLayout[] = {
    // Per Vertex data
    vk::VertexInputAttributeDescription(
        0,
        0,
        vk::Format::eR32G32Sfloat,
        offsetof(Vertex, pos)),
    vk::VertexInputAttributeDescription(
        1,
        0,
        vk::Format::eR32G32B32Sfloat,
        offsetof(Vertex, color)),
    vk::VertexInputAttributeDescription(
        2,
        0,
        vk::Format::eR32G32Sfloat,
        offsetof(Vertex, texCoord)),
    // Per Instance Data
    vk::VertexInputAttributeDescription(
        3,
        1,
        vk::Format::eR32G32Sfloat,
        offsetof(Instance, pos)),
  };

  /*
   * bindingLayout
   *  Defines the binding layout for all shaders
   */
  static constexpr vk::VertexInputBindingDescription bindingLayout[] = {
    vk::VertexInputBindingDescription(
        0,
        sizeof(Vertex),
        vk::VertexInputRate::eVertex),
    vk::VertexInputBindingDescription(
        1,
        sizeof(Instance),
        vk::VertexInputRate::eInstance)
  };

  /* Mesh
   *  Storage of vertex, index, and layout info to draw a type of object
   *  This definition of a mesh is limited, textures & submeshes 
   *  are not supported yet
   */
  struct Mesh {
    Handle<Vertex> vertices;
    Handle<uint16_t> indices;
  };

  /*
   * Camera
   *  General CPU calculated camera class
   *  This class computes the ViewProjection matrix for rendering a scene
   *  TODO: move the computation to the GPU
   */
  class Camera {
    public:
      glm::vec3 pos;
      glm::float64 pitch, yaw, roll;
      
      glm::mat4 view, proj, cam;

      Camera(vk::Extent2D extent)
      {
        // Set projection matrix
        proj = glm::perspective(
            glm::radians(90.0),
            (glm::float64) (extent.width / extent.height),
            0.1,
            1000.0);
        
        // Set initial pos, pitch and yaw
        jump(glm::vec3(0.0, 0.0, .1), 0, 0, 0);
        build();
      }

      void move(
          glm::vec3 deltaPos,
          glm::float64 deltaPitch,
          glm::float64 deltaYaw,
          glm::float64 deltaRoll)
      {
        pos += deltaPos;
        pitch += deltaPitch;
        yaw += deltaYaw;
        roll += deltaRoll;
      }

      void jump(
          glm::vec3 newPos,
          glm::float64 newPitch,
          glm::float64 newYaw,
          glm::float64 newRoll)
      {
        pos = newPos;
        pitch = newPitch;
        yaw = newYaw;
        roll = newRoll;
      }

      // Credit to https://www.3dgep.com/understanding-the-view-matrix/
      // TODO:: Support changes to projection matrix (aspect resize mainly)
      void build()
      {
        pitch = glm::min(pitch, glm::radians(90.0));
        pitch = glm::max(pitch, glm::radians(-90.0));

        if (yaw < glm::radians(0.0))
        {
          yaw += glm::radians(360.0);
        }
        if (yaw > glm::radians(360.0))
        {
          yaw -= glm::radians(360.0);
        }

        if (roll < glm::radians(0.0)) {
          roll += glm::radians(360.0);
        } if (roll > glm::radians(360.0)) {
          roll -= glm::radians(360.0);
        }

        // Pitch = rotation about x axis
        // yaw = rotation about y axis
        // roll = rotation about z axis

        glm::float64 cosPitch = glm::cos(pitch);
        glm::float64 sinPitch = glm::sin(pitch);
        glm::float64 cosYaw = glm::cos(yaw);
        glm::float64 sinYaw = glm::sin(yaw);
        glm::float64 cosRoll = glm::cos(roll);
        glm::float64 sinRoll = glm::sin(roll);
        
        // Compute 3 axis rotation matrix (RxRyRz)
        glm::vec3 x = {
          cosYaw * cosRoll,
          sinPitch * sinYaw * cosRoll + cosPitch * sinRoll, 
          -(cosPitch * sinYaw * cosRoll) + sinPitch * sinRoll
        };
        glm::vec3 y = {
          -cosYaw * sinRoll,
          -(sinPitch * sinYaw * sinRoll) + cosPitch * cosRoll,
          cosPitch * sinYaw * sinRoll + sinPitch * cosRoll
        };
        glm::vec3 z = {
          sinYaw,
          -(sinPitch * cosYaw),
          cosPitch * cosYaw
        };

        // Invert via transpose
        view = glm::mat4(
            glm::vec4(x.x, y.x, z.x, 0),
            glm::vec4(x.y, y.y, z.y, 0),
            glm::vec4(x.z, y.z, z.z, 0),
            glm::vec4(-glm::dot(x, pos), -glm::dot(y, pos), -glm::dot(z, pos), 1));
        
        cam = proj * view;
      }
  };

  /* RenderTarget
   *  Stores data for drawing the scene to an image from a given perspective
   *  Intermediate render targets are not supported 
   */
  struct RenderTarget {
  
  };
  
  /*
   * Context
   *  The context is the base class of ngfx, storing all the core constructs
   *  required by vulkan
   */
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

  /* gBuffer
   *  Abstracts GPU buffer and transfer semantics and provides
   *  fast transfers between the GPU and CPU
   */
  struct gBuffer 
  {
    bool valid;
    Context *c;
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingMemory;
    vk::Buffer localBuffer;
    vk::DeviceMemory localMemory;
    vk::CommandBuffer commandBuffer;

    gBuffer(Context *context);

    void init(vk::DeviceSize size, vk::BufferUsageFlags usage);
    void map(void* data);
    void unmap();
    void copy();
    void blockingCopy();
    ~gBuffer(void);
  };

  /*
   * Scene
   *  Core struct for rendering mesh objects to render targets
   */
  struct Scene
    {
      // Handles to CPU side resources
      Handle<Vertex> vertices;
      Handle<uint16_t> indices;
      Handle<vk::DeviceSize> vertexOffsets;
      Handle<vk::DeviceSize> indexOffsets;
      Handle<vk::DrawIndexedIndirectCommand> drawCmds;
      // TODO: Determine if there is a way to use mapping to remove need
      // for the unified Instances buffer
      Handle<Handle<Instance>> instances;
      Handle<Instance> unifiedInstances;
      Handle<Camera> cameras;
      Handle<RenderTarget> targets;

      // Buffers for GPU side resources
      gBuffer vertexBuffer;
      gBuffer indexBuffer;
      gBuffer drawCmdBufffer;
      gBuffer instanceBuffer;
      gBuffer cameraBuffer;
      gBuffer targetBuffer;

      vk::RenderPass pass;
      std::vector<vk::Framebuffer> frames;
      vk::PipelineLayout layout;
      vk::Pipeline pipeline;
      
      vk::DescriptorSetLayout descLayout;
      vk::DescriptorPool descPool;
      vk::DescriptorSet descSet;

      // Pointer for device held for use in destructor only
      // Pointer must stay valid for lifetime
      Context *c;

      Scene(Context *c);
      void loadMeshes(Handle<Mesh> meshes);
      ~Scene();

      private:
        void createDescriptorPool(void);
        void createDescriptorSets(void);
    };
}
#endif // NGFX_H

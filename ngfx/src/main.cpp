// TODO: move surface define into cmake
#include <fstream>
#include <iostream>
#include <thread>
#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "utils.hpp"
#include "math.hpp"
#include "image.hpp"
#include "time.h"

// TODO: remove these when not needed
#include <thread>
#include <chrono>


/*
 * 2d rendering library with a heavy emphasis on multiviewpoint and headless rendering, intended for use with nenbody project
 *
 * The code here is heavily borrowed from:
 * https://github.com/KhronosGroup/Vulkan-Hpp/tree/master/samples
 * https://github.com/SaschaWillems/Vulkan
 */

#define LOG(...) printf(__VA_ARGS__)
namespace ngfx
{
    struct Vertex
    {
        float x, y, z, w;   // Position
        float r, g, b, a;   // Color
    };

    static Vertex kTestData[] =
    {
      // red face
      { -1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
      { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
      {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
      {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
      { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
      {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
      // green face
      { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
      {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
      { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
      { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
      {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
      {  1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
      // blue face
      { -1.0f,  1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
      { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
      { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
      { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
      { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
      { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
      // yellow face
      {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
      {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
      {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
      {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
      {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
      {  1.0f, -1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
      // magenta face
      {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
      { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
      {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
      {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
      { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
      { -1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
      // cyan face
      {  1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
      {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
      { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
      { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
      {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
      { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
    };

    vk::UniqueShaderModule loadShader(const char *fileName, vk::UniqueDevice &device)
    {
        std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);
        vk::UniqueShaderModule shaderModule;
        if (is.is_open())
        {
            std::streamoff size = is.tellg();
            is.seekg(0, std::ios::beg);
            char* shaderCode = new char[(size_t) size];
            is.read(shaderCode, size);
            is.close();
            vk::ShaderModuleCreateInfo moduleCreateInfo{};
            moduleCreateInfo.codeSize = (size_t) size;
            moduleCreateInfo.pCode = (uint32_t*) shaderCode;
            shaderModule = device->createShaderModuleUnique(moduleCreateInfo);
            delete[] shaderCode;
        }
        else
        {
            std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << std::endl;
        }
        return shaderModule;
    }

    // TODO: Docs
    class Renderer
    {
    public:
        static const uint32_t kArraylayers = 1;

        struct UBO {
            glm::mat4 projection[kArraylayers];
            glm::mat4 modelview[kArraylayers];
        } ubo;
        // TODO: Add array of instanced drawables renderer input format
        Renderer(Vertex *geometry, size_t geometrySize)
        {
            init(500, 500);
            buildImages();
            buildOffscreenRenderpass();
            buildDisplayRenderpass();
            buildFrameBuffers();
            buildUbo();
            buildDescriptors();
            
            _vertexBufferData = new vk::su::BufferData(_physicalDevice,
                                                _device,
                                                geometrySize,
                                                vk::BufferUsageFlagBits::eVertexBuffer);

            vk::su::copyToDevice(_device,
                                 _vertexBufferData->deviceMemory,
                                 geometry,
                                 geometrySize / sizeof(geometry[0]));

            buildPipelines();
            buildCommandBuffers();

            vk::SemaphoreCreateInfo semaphoreCI{};

            semaphores.presentComplete = _device->createSemaphoreUnique(semaphoreCI);
            semaphores.renderComplete = _device->createSemaphoreUnique(semaphoreCI);

        }

        void draw(void)
        {
            // Get the index of the next available swapchain image:
            uint32_t currentBuffer;
            vk::Result res = _device->acquireNextImageKHR(_swapChainData->swapChain.get(),
                                                                                   vk::su::FenceTimeout,
                                                                                   semaphores.presentComplete.get(),
                                                                                   nullptr,
                                                                                   &currentBuffer);
            _device->waitForFences(1, &_waitFences[currentBuffer].get(), 1, vk::su::FenceTimeout);
            _device->resetFences(1, &_waitFences[currentBuffer].get());

            // TODO: add window resize
            assert(res == vk::Result::eSuccess);


            vk::SubmitInfo submitInfo(1,
                                      &semaphores.presentComplete.get(),
                                      &_submitPipelineStages,
                                      1,
                                      &_drawCmdBuffers[currentBuffer].get(),
                                      1,
                                      &semaphores.renderComplete.get()
                                      );

            _graphicsQueue.submit(1, &submitInfo, _waitFences[currentBuffer].get());

            // TODO: validation/window resize checking
            _graphicsQueue.presentKHR(vk::PresentInfoKHR(0,
                                                        &semaphores.renderComplete.get(),
                                                        1,
                                                        &_swapChainData->swapChain.get(),
                                                        &currentBuffer)
                                     );
            _graphicsQueue.waitIdle();
        }

        void init(uint32_t width, uint32_t height)
        {
            // Initialize instance, error messaging, and physical deviee
            _instance = vk::su::createInstance("ngfx",
                                               "Vulkan-Hpp",
                                               {},
                                               vk::su::getInstanceExtensions(),
                                               VK_API_VERSION_1_1
                                               );

            _debugUtilsMessenger = vk::su::createDebugUtilsMessenger(_instance);
            _physicalDevice = _instance->enumeratePhysicalDevices().front();

            vk::PhysicalDeviceProperties props = _physicalDevice.getProperties();

            std::cout << "Max Array Layers: " << props.limits.maxImageArrayLayers << std::endl;

            _onscreenExtent = vk::Extent2D(width, height);
            _offscreenExtent = vk::Extent2D(width, height);

            _surfaceData = new vk::su::SurfaceData(_instance, "ngfx", _onscreenExtent);

            _queueFamilyIndicies = vk::su::findGraphicsAndPresentQueueFamilyIndex(_physicalDevice,
                                                                                  *_surfaceData->surface
                                                                                  );
            _device = vk::su::createDevice(_physicalDevice,
                                           _queueFamilyIndicies.first,
                                           vk::su::getDeviceExtensions());

            _commandPool = vk::su::createCommandPool(_device, _queueFamilyIndicies.first);

            _graphicsQueue = _device->getQueue(_queueFamilyIndicies.first, 0);
            _presentQueue = _device->getQueue(_queueFamilyIndicies.second, 0);

            _swapChainData = new vk::su::SwapChainData(_physicalDevice,
                                                _device,
                                                *(_surfaceData->surface),
                                                _surfaceData->extent,
                                                vk::ImageUsageFlagBits::eColorAttachment
                                                    | vk::ImageUsageFlagBits::eTransferDst,
                                                vk::UniqueSwapchainKHR(),
                                                _queueFamilyIndicies.first,
                                                _queueFamilyIndicies.second);



            _drawCmdBuffers =_device->allocateCommandBuffersUnique(
                                           vk::CommandBufferAllocateInfo(_commandPool.get(),
                                           vk::CommandBufferLevel::ePrimary,
                                           static_cast<uint32_t>(_swapChainData->images.size())
                                           ));

            _waitFences.resize(_drawCmdBuffers.size());
            for (uint i = 0; i<_drawCmdBuffers.size(); i++)
            {
                _waitFences[i] = _device->createFenceUnique(vk::FenceCreateInfo());
            }

            _depthBufferData = new vk::su::DepthBufferData(_physicalDevice,
                                                    _device,
                                                    vk::Format::eD16Unorm,
                                                    _surfaceData->extent);

            _pipelineCache = _device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo());

            _submitPipelineStages =vk::PipelineStageFlagBits::eColorAttachmentOutput;
        }


        
    private:

        vk::UniqueInstance _instance;
        vk::UniqueDebugUtilsMessengerEXT _debugUtilsMessenger;
        vk::PhysicalDevice _physicalDevice;
        std::pair<uint32_t, uint32_t> _queueFamilyIndicies; // {Graphics, Present}
        vk::UniqueDevice _device;
        vk::UniqueCommandPool _commandPool;

        struct {
            // Swap chain image presentation
            vk::UniqueSemaphore presentComplete;
            // Command buffer submission and execution
            vk::UniqueSemaphore renderComplete;
        } semaphores;

        vk::Extent2D _onscreenExtent;
        vk::Extent2D _offscreenExtent;

        vk::UniqueImage _colorImage;
        vk::UniqueImage _depthImage;

        vk::UniqueImageView _colorView;
        vk::UniqueImageView _depthView;

        vk::DeviceMemory _colorMemory;
        vk::DeviceMemory _depthMemory;

        vk::UniqueSampler _imageSampler;

        // TODO: allocate Data classes better
        vk::su::SwapChainData *_swapChainData;
        vk::su::SurfaceData *_surfaceData;
        vk::su::DepthBufferData *_depthBufferData;
        vk::su::BufferData *_uniformBufferData;
        vk::su::BufferData *_vertexBufferData;

        std::vector<vk::UniqueCommandBuffer> _drawCmdBuffers;
        std::vector<vk::UniqueFence> _waitFences;

        vk::Queue _graphicsQueue;
        vk::Queue _presentQueue;

        vk::UniqueDescriptorSetLayout _descriptorSetLayout;
        vk::UniqueRenderPass _displayRenderPass;
        vk::UniqueRenderPass _offscreenRenderPass;
        vk::UniqueShaderModule _vertexShaderModule;
        vk::UniqueShaderModule _fragmentShaderModule;
        vk::UniqueShaderModule _samplerShaderModule;
        std::vector<vk::UniqueFramebuffer> _displayFramebuffers;
        vk::UniqueFramebuffer _offscreenFramebuffer;
        vk::UniqueDescriptorPool _descriptorPool;
        vk::UniqueDescriptorSet _descriptorSet;
        vk::UniquePipelineLayout _pipelineLayout;
        vk::UniquePipelineCache _pipelineCache;
        vk::UniquePipeline _offscreenPipeline;
        vk::UniquePipeline _displayPipeline;
        vk::UniquePipeline _debugPipeline;

        vk::PipelineStageFlags _submitPipelineStages;

        vk::Bool32 getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties, uint32_t* typeIndex) const
        {
            for (uint32_t i = 0; i < 32; i++) {
                if ((typeBits & 1) == 1) {
                    if ((_physicalDevice.getMemoryProperties().memoryTypes[i].propertyFlags & properties) == properties) {
                        *typeIndex = i;
                        return VK_TRUE;
                    }
                }
                typeBits >>= 1;
            }
            return VK_FALSE;
        }

        uint32_t getMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties) const
        {
               uint32_t result = 0;
               if (VK_FALSE == getMemoryType(typeBits, properties, &result)) {
                   throw std::runtime_error("Unable to find memory type " + vk::to_string(properties));
                   // todo : throw error
               }
               return result;
           }

        vk::Format getSupportedDepthFormat() const
        {
            // Since all depth formats may be optional, we need to find a suitable depth format to use
            // Start with the highest precision packed format
            std::vector<vk::Format> depthFormats = { vk::Format::eD16Unorm, vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16UnormS8Uint,
                                                     };

            for (auto& format : depthFormats) {
                vk::FormatProperties formatProps;
                formatProps = _physicalDevice.getFormatProperties(format);
                // vk::Format must support depth stencil attachment for optimal tiling
                if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                    return format;
                }
            }
            throw std::runtime_error("No supported depth format");
        }

        void buildCommandBuffers(void)
        {
            vk::CommandBufferBeginInfo cmdBuffCI(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

            for (size_t i = 0; i < _swapChainData->images.size(); ++i)
            {
                vk::UniqueCommandBuffer &cmdBuffer = _drawCmdBuffers[i];

                // First render pass: display
                vk::ClearValue clearValues[2];
                clearValues[0].color = vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
                clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

                cmdBuffer->begin(cmdBuffCI);

                // TODO: multiple passes for different framebuffers?
                vk::RenderPassBeginInfo renderPassBeginInfo(_displayRenderPass.get(),
                                                            _displayFramebuffers[i].get(),
                                                            vk::Rect2D(vk::Offset2D(0, 0),
                                                                _onscreenExtent),
                                                            2,
                                                            clearValues);
                cmdBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
                cmdBuffer->setViewport(0, vk::Viewport(0.0f,
                                                            0.0f,
                                                            static_cast<float>(_onscreenExtent.width),
                                                            static_cast<float>(_onscreenExtent.height),
                                                            0.0f,
                                                            1.0f));
                cmdBuffer->setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _onscreenExtent));

                cmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, _displayPipeline.get());
                cmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout.get(),
                                               0,
                                               _descriptorSet.get(),
                                               nullptr);

                cmdBuffer->bindVertexBuffers(0, *(_vertexBufferData->buffer), {0});

                // TODO: Indexed draw possibly needed later on?
                cmdBuffer->draw(12 * 3, 1, 0, 0);
                cmdBuffer->endRenderPass();
                cmdBuffer->end();


            }
        }

        void buildImages(void)
        {
            vk::ImageCreateInfo depthCI(vk::ImageCreateFlags(),
                                           vk::ImageType::e2D,
                                           _depthBufferData->format,
                                           vk::Extent3D(
                                             _offscreenExtent.width,
                                             _offscreenExtent.height,
                                             1
                                           ),
                                           1,
                                           kArraylayers,
                                           vk::SampleCountFlagBits::e1,
                                           vk::ImageTiling::eOptimal,
                                           vk::ImageUsageFlagBits::eDepthStencilAttachment
                                        );
            _depthImage = _device->createImageUnique(depthCI);
            vk::MemoryRequirements depthMemReqs = _device->getImageMemoryRequirements(_depthImage.get());
            vk::MemoryAllocateInfo depthAllocInfo(depthMemReqs.size,
                                                getMemoryType(
                                                    depthMemReqs.memoryTypeBits,
                                                    vk::MemoryPropertyFlags()
                                                ));
            _depthMemory = _device->allocateMemory(depthAllocInfo);
            _device->bindImageMemory(_depthImage.get(), _depthMemory, 0);
            vk::ImageViewCreateInfo depthViewCI(vk::ImageViewCreateFlags(),
                                                _depthImage.get(),
                                                vk::ImageViewType::e2DArray,
                                                _depthBufferData->format,
                                                {},
                                                vk::ImageSubresourceRange(
                                                    vk::ImageAspectFlagBits::eDepth,
                                                    0,
                                                    1,
                                                    0,
                                                    kArraylayers
                                                ));
            _depthView = _device->createImageViewUnique(depthViewCI);


            vk::ImageCreateInfo colorCI(vk::ImageCreateFlags(),
                                        vk::ImageType::e2D,
                                        _swapChainData->colorFormat,
                                        vk::Extent3D(
                                            _offscreenExtent.width,
                                            _offscreenExtent.height,
                                            1
                                        ),
                                        1,
                                        kArraylayers,
                                        vk::SampleCountFlagBits::e1,
                                        vk::ImageTiling::eOptimal,
                                        vk::ImageUsageFlagBits::eColorAttachment
                                            | vk::ImageUsageFlagBits::eSampled
                                        );
            _colorImage = _device->createImageUnique(colorCI);
            vk::MemoryRequirements colorMemReqs = _device->getImageMemoryRequirements(_colorImage.get());
            vk::MemoryAllocateInfo colorAllocInfo(colorMemReqs.size,
                                                getMemoryType(
                                                    colorMemReqs.memoryTypeBits,
                                                    vk::MemoryPropertyFlags()
                                                ));
            _colorMemory = _device->allocateMemory(colorAllocInfo);
            _device->bindImageMemory(_colorImage.get(), _colorMemory, 0);
            vk::ImageViewCreateInfo colorViewCI(vk::ImageViewCreateFlags(),
                                                _colorImage.get(),
                                                //TODO: 2D array?
                                                vk::ImageViewType::e2D,
                                                _swapChainData->colorFormat,
                                                {},
                                                vk::ImageSubresourceRange(
                                                    vk::ImageAspectFlagBits::eColor,
                                                    0,
                                                    1,
                                                    0,
                                                    kArraylayers
                                                ));
            _colorView = _device->createImageViewUnique(colorViewCI);

            // Create Sampler for color image

            vk::SamplerCreateInfo samplerCI(vk::SamplerCreateFlags(),
                                            vk::Filter::eLinear,
                                            vk::Filter::eLinear,
                                            vk::SamplerMipmapMode::eLinear,
                                            vk::SamplerAddressMode::eClampToEdge,
                                            vk::SamplerAddressMode::eClampToEdge,
                                            vk::SamplerAddressMode::eClampToEdge,
                                            0.0f,
                                            false,
                                            0.0,
                                            false,
                                            vk::CompareOp::eNever,
                                            0.0f,
                                            1.0f,
                                            vk::BorderColor::eIntOpaqueWhite
                                            );
            _imageSampler = _device->createSamplerUnique(samplerCI);

        }

        void buildOffscreenRenderpass(void)
        {
            vk::AttachmentDescription colorAttachment(vk::AttachmentDescriptionFlags(),
                                                      _swapChainData->colorFormat,
                                                      vk::SampleCountFlagBits::e1,
                                                      vk::AttachmentLoadOp::eClear,
                                                      vk::AttachmentStoreOp::eStore,
                                                      vk::AttachmentLoadOp::eDontCare,
                                                      vk::AttachmentStoreOp::eDontCare,
                                                      vk::ImageLayout::eUndefined, // initial
                                                      vk::ImageLayout::eShaderReadOnlyOptimal // final
                                                      );
            vk::AttachmentDescription depthAttachment(vk::AttachmentDescriptionFlags(),
                                                      _depthBufferData->format,
                                                      vk::SampleCountFlagBits::e1,
                                                      vk::AttachmentLoadOp::eClear,
                                                      vk::AttachmentStoreOp::eStore,
                                                      vk::AttachmentLoadOp::eClear,
                                                      vk::AttachmentStoreOp::eDontCare,
                                                      vk::ImageLayout::eUndefined, // initial
                                                      vk::ImageLayout::eDepthStencilAttachmentOptimal // final
                                                      );
            vk::AttachmentDescription attachments[2] =
            {
                colorAttachment,
                depthAttachment
            };

            vk::AttachmentReference colorReference(0,
                                                   vk::ImageLayout::eColorAttachmentOptimal
                                                   );
            vk::AttachmentReference depthReference(1,
                                                   vk::ImageLayout::eDepthStencilAttachmentOptimal
                                                   );

            vk::SubpassDescription subpassDesc(vk::SubpassDescriptionFlags(),
                                               vk::PipelineBindPoint::eGraphics,
                                               0,
                                               nullptr,
                                               1,
                                               &colorReference,
                                               nullptr,
                                               &depthReference,
                                               0,
                                               nullptr
                                               );

            vk::SubpassDependency colorDependency(VK_SUBPASS_EXTERNAL,
                                                  0,
                                                  vk::PipelineStageFlagBits::eFragmentShader,
                                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                  vk::AccessFlagBits::eShaderRead,
                                                  vk::AccessFlagBits::eColorAttachmentWrite,
                                                  vk::DependencyFlagBits::eByRegion
                                                  );
            vk::SubpassDependency depthDependency(0,
                                                  VK_SUBPASS_EXTERNAL,
                                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                  vk::PipelineStageFlagBits::eFragmentShader,
                                                  vk::AccessFlagBits::eColorAttachmentWrite,
                                                  vk::AccessFlagBits::eShaderRead,
                                                  vk::DependencyFlagBits::eByRegion
                                                  );
            vk::SubpassDependency dependencies[2] =
            {
                colorDependency,
                depthDependency
            };

            vk::RenderPassCreateInfo offscreenCI(vk::RenderPassCreateFlags(),
                                                  2,
                                                  attachments,
                                                  1,
                                                  &subpassDesc,
                                                  2,
                                                  dependencies
                                                  );

            _offscreenRenderPass = _device->createRenderPassUnique(offscreenCI);


        }

        void buildDisplayRenderpass(void)
        {
            vk::AttachmentDescription colorAttachment(vk::AttachmentDescriptionFlags(),
                                                      _swapChainData->colorFormat,
                                                      vk::SampleCountFlagBits::e1,
                                                      vk::AttachmentLoadOp::eClear,
                                                      vk::AttachmentStoreOp::eStore,
                                                      vk::AttachmentLoadOp::eDontCare,
                                                      vk::AttachmentStoreOp::eDontCare,
                                                      vk::ImageLayout::eUndefined, // initial
                                                      vk::ImageLayout::ePresentSrcKHR // final
                                                      );
            vk::AttachmentDescription depthAttachment(vk::AttachmentDescriptionFlags(),
                                                      _depthBufferData->format,
                                                      vk::SampleCountFlagBits::e1,
                                                      vk::AttachmentLoadOp::eClear,
                                                      vk::AttachmentStoreOp::eStore,
                                                      vk::AttachmentLoadOp::eClear,
                                                      vk::AttachmentStoreOp::eDontCare,
                                                      vk::ImageLayout::eUndefined, // initial
                                                      vk::ImageLayout::eDepthStencilAttachmentOptimal // final
                                                      );
            vk::AttachmentDescription attachments[2] =
            {
                colorAttachment,
                depthAttachment
            };

            vk::AttachmentReference colorReference(0,
                                                   vk::ImageLayout::eColorAttachmentOptimal
                                                   );
            vk::AttachmentReference depthReference(1,
                                                   vk::ImageLayout::eDepthStencilAttachmentOptimal
                                                   );

            vk::SubpassDescription subpassDesc(vk::SubpassDescriptionFlags(),
                                               vk::PipelineBindPoint::eGraphics,
                                               0,
                                               nullptr,
                                               1,
                                               &colorReference,
                                               nullptr,
                                               &depthReference,
                                               0,
                                               nullptr
                                               );

            vk::SubpassDependency colorDependency(VK_SUBPASS_EXTERNAL,
                                                  0,
                                                  vk::PipelineStageFlagBits::eBottomOfPipe,
                                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                  vk::AccessFlagBits::eMemoryRead,
                                                  vk::AccessFlagBits::eColorAttachmentRead
                                                    | vk::AccessFlagBits::eColorAttachmentWrite,
                                                  vk::DependencyFlagBits::eByRegion
                                                  );
            vk::SubpassDependency depthDependency(0,
                                                  VK_SUBPASS_EXTERNAL,
                                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                  vk::PipelineStageFlagBits::eBottomOfPipe,
                                                  vk::AccessFlagBits::eColorAttachmentRead
                                                    | vk::AccessFlagBits::eColorAttachmentWrite,
                                                  vk::AccessFlagBits::eMemoryRead,
                                                  vk::DependencyFlagBits::eByRegion
                                                  );
            vk::SubpassDependency dependencies[2] =
            {
                colorDependency,
                depthDependency
            };

            vk::RenderPassCreateInfo displayCI(vk::RenderPassCreateFlags(),
                                                  2,
                                                  attachments,
                                                  1,
                                                  &subpassDesc,
                                                  2,
                                                  dependencies
                                                  );

            _displayRenderPass = _device->createRenderPassUnique(displayCI);

         }

        void buildFrameBuffers(void)
        {
            vk::ImageView attachments[2] =
            {
                _colorView.get(),
                _depthView.get()
            };

            // Offscreen framebuffers for each target image
            // TODO: more than one offscreen image
            vk::FramebufferCreateInfo offscreenCI(vk::FramebufferCreateFlags(),
                                                    _offscreenRenderPass.get(),
                                                    2,
                                                    attachments,
                                                    _offscreenExtent.width,
                                                    _offscreenExtent.height,
                                                    1);
            _offscreenFramebuffer = _device->createFramebufferUnique(offscreenCI);


            // Display framebuffers for each swapchain image
            _displayFramebuffers.resize(_swapChainData->images.size());
            vk::FramebufferCreateInfo displayCI(vk::FramebufferCreateFlags(),
                                                _displayRenderPass.get(),
                                                2,
                                                attachments,
                                                _onscreenExtent.width,
                                                _onscreenExtent.height,
                                                1);
            for(uint i = 0; i < _swapChainData->images.size(); i++)
            {
                attachments[0] = _swapChainData->imageViews[i].get();
                _displayFramebuffers[i] = _device->createFramebufferUnique(displayCI);
            }

        }

        void buildUbo(void)
        {
            _uniformBufferData = new vk::su::BufferData(_physicalDevice,
                                                 _device,
                                                 sizeof(ubo),
                                                 vk::BufferUsageFlagBits::eUniformBuffer);
            // TODO: update UBO
            vk::su::copyToDevice(_device,
                                 _uniformBufferData->deviceMemory,
                                 // TODO: Custom MVP
                                 vk::su::createModelViewProjectionClipMatrix(_surfaceData->extent));
        }

        void buildDescriptors(void)
        {

            std::vector<vk::DescriptorPoolSize> poolSizes{
                { vk::DescriptorType::eUniformBuffer, 6 },
                { vk::DescriptorType::eCombinedImageSampler, 8 },
            };
            _descriptorPool = _device->createDescriptorPoolUnique({
                                                                   {},
                                                                   5,
                                                                   (uint32_t)poolSizes.size(),
                                                                   poolSizes.data()
                                                                 });


            std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
                // Binding 0 : Vertex shader uniform buffer
                { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
                // Binding 1 : Fragment shader image sampler
                { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            };

            _descriptorSetLayout = _device->createDescriptorSetLayoutUnique({
                                                                   {},
                                                                   (uint32_t)setLayoutBindings.size(),
                                                                   setLayoutBindings.data()
                                                                   });


            _descriptorSet = std::move(_device->allocateDescriptorSetsUnique(
                                           vk::DescriptorSetAllocateInfo(
                                               *_descriptorPool,
                                               1,
                                               &*_descriptorSetLayout)
                                           ).front()
                                       );

            // UBO descriptor for the MVP matrix
            vk::DescriptorBufferInfo descBufInfo(_uniformBufferData->buffer.get(),
                                     0,
                                     sizeof(ubo)
                                    );

            // vk::Image descriptor for the image sampler
            vk::DescriptorImageInfo descImageInfo(_imageSampler.get(),
                                                  _colorView.get(),
                                                  vk::ImageLayout::eShaderReadOnlyOptimal
                                                 );

            vk::WriteDescriptorSet descriptorSets[2] =
            {
                vk::WriteDescriptorSet(_descriptorSet.get(),
                                       0,
                                       0,
                                       1,
                                       vk::DescriptorType::eUniformBuffer,
                                       nullptr,
                                       &descBufInfo
                                      ),
                vk::WriteDescriptorSet(_descriptorSet.get(),
                                       1,
                                       0,
                                       1,
                                       vk::DescriptorType::eCombinedImageSampler,
                                       &descImageInfo
                                      )
            };

            _device->updateDescriptorSets(2, descriptorSets, 0, nullptr);
        }

        void buildPipelines(void)
        {
            _pipelineLayout = _device->createPipelineLayoutUnique(
                        vk::PipelineLayoutCreateInfo(
                            vk::PipelineLayoutCreateFlags(),
                            1,
                            &_descriptorSetLayout.get()
                            )
                        );

            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState(vk::PipelineInputAssemblyStateCreateFlags(),
                                                                          vk::PrimitiveTopology::eLineStrip,
                                                                          false
                                                                        );

            vk::PipelineRasterizationStateCreateInfo rasterizationState(vk::PipelineRasterizationStateCreateFlags(),
                                                                        false,
                                                                        false,
                                                                        vk::PolygonMode::eFill,
                                                                        vk::CullModeFlagBits::eFront,
                                                                        vk::FrontFace::eClockwise,
                                                                        false,
                                                                        0.0,
                                                                        0.0,
                                                                        0.0,
                                                                        1.0 // Line Width
                                                                        );

            vk::PipelineColorBlendAttachmentState blendAttachmentState(false,
                                                                       vk::BlendFactor::eZero,
                                                                       vk::BlendFactor::eZero,
                                                                       vk::BlendOp::eAdd,
                                                                       vk::BlendFactor::eZero,
                                                                       vk::BlendFactor::eZero,
                                                                       vk::BlendOp::eAdd,
                                                                       vk::ColorComponentFlagBits::eA
                                                                        | vk::ColorComponentFlagBits::eB
                                                                        | vk::ColorComponentFlagBits::eG
                                                                        | vk::ColorComponentFlagBits::eR
                                                                       );

            vk::PipelineColorBlendStateCreateInfo colorBlendState(vk::PipelineColorBlendStateCreateFlags(),
                                                                  false,
                                                                  vk::LogicOp::eClear,
                                                                  1,
                                                                  &blendAttachmentState
                                                                  );

            vk::PipelineDepthStencilStateCreateInfo depthStencilState(vk::PipelineDepthStencilStateCreateFlags(),
                                                                      true,
                                                                      true,
                                                                      vk::CompareOp::eLessOrEqual
                                                                      );

            // TODO: Null initialized viewports at the start, should have something ready to pass here
            vk::PipelineViewportStateCreateInfo viewportState(vk::PipelineViewportStateCreateFlags(),
                                                              1,
                                                              nullptr,
                                                              1,
                                                              nullptr
                                                              );

            vk::PipelineMultisampleStateCreateInfo multisampleState(vk::PipelineMultisampleStateCreateFlags(),
                                                                    vk::SampleCountFlagBits::e1
                                                                    );

            vk::DynamicState dynamicStateEnables[2] =
            {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor
            };

            vk::PipelineDynamicStateCreateInfo dynamicState(vk::PipelineDynamicStateCreateFlags(),
                                                            2,
                                                            dynamicStateEnables
                                                            );

            _vertexShaderModule = ngfx::loadShader("shaders/vert.spv", _device);
            _fragmentShaderModule = ngfx::loadShader("shaders/frag.spv", _device);

            vk::PipelineShaderStageCreateInfo shaderStages[2] =
            {
                vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                                  vk::ShaderStageFlagBits::eVertex,
                                                  _vertexShaderModule.get(),
                                                  "main"
                                                  ),
                vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                                  vk::ShaderStageFlagBits::eFragment,
                                                  _fragmentShaderModule.get(),
                                                  "main"
                                                  )
            };
            vk::VertexInputBindingDescription vertexInputBinding(0,
                                                                 sizeof(Vertex),
                                                                 vk::VertexInputRate::eVertex
                                                                 );

            // Vertex input attributes
            vk::VertexInputAttributeDescription vertexInputAttributes[2] =
            {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0), // position
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, sizeof(float) * 4), // color
            };

            vk::PipelineVertexInputStateCreateInfo vertexInputState(vk::PipelineVertexInputStateCreateFlags(),
                                                                    1,
                                                                    &vertexInputBinding,
                                                                    2,
                                                                    vertexInputAttributes
                                                                    );

            vk::GraphicsPipelineCreateInfo pipelineCI(vk::PipelineCreateFlags(),
                                                      2,
                                                      shaderStages,
                                                      &vertexInputState,
                                                      &inputAssemblyState,
                                                      nullptr,
                                                      &viewportState,
                                                      &rasterizationState,
                                                      &multisampleState,
                                                      &depthStencilState,
                                                      &colorBlendState,
                                                      &dynamicState,
                                                      _pipelineLayout.get(),
                                                      _offscreenRenderPass.get()
                                                      );

            _offscreenPipeline = _device->createGraphicsPipelineUnique(_pipelineCache.get(), pipelineCI);
            
            _displayPipeline = _device->createGraphicsPipelineUnique(_pipelineCache.get(), pipelineCI);

            shaderStages[0] = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                                                vk::ShaderStageFlagBits::eVertex,
                                                                _vertexShaderModule.get(),
                                                                "main"
                                                                );

            shaderStages[1] = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                                                vk::ShaderStageFlagBits::eFragment,
                                                                _fragmentShaderModule.get(),
                                                                "main"
                                                                );

            _debugPipeline = _device->createGraphicsPipelineUnique(_pipelineCache.get(), pipelineCI);
        }

    };
}

int main(int /*argc*/, char ** /*argv*/)
{
    ngfx::Renderer gfx(ngfx::kTestData, sizeof(ngfx::kTestData));

    std::chrono::milliseconds timespan(1000);
    while (1)
    {
        gfx.draw();
        std::this_thread::sleep_for(timespan);
    }
}

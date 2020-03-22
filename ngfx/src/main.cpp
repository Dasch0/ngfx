// TODO: move surface define into cmake
#include <fstream>
#include <iostream>
#include <thread>
#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"
#include "../../lib/Vulkan-Hpp/samples/utils/utils.hpp"
#include "../../lib/Vulkan-Hpp/samples/utils/math.hpp"


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


    // TODO: Docs
    class Renderer
    {
    public:
        // TODO: Add array of instanced drawables renderer input format
        Renderer(Vertex *geometry, size_t geometrySize)
        {
            // TODO: Add multiview extension
            _instance = vk::su::createInstance("ngfx", "Vulkan-Hpp", {}, vk::su::getInstanceExtensions(), VK_API_VERSION_1_1);
            _debugUtilsMessenger = vk::su::createDebugUtilsMessenger(_instance);
            _physicalDevice = _instance->enumeratePhysicalDevices().front();

            _surfaceData = new vk::su::SurfaceData(_instance, "ngfx", vk::Extent2D(500, 500));

            _queueFamilyIndicies = vk::su::findGraphicsAndPresentQueueFamilyIndex(_physicalDevice, *_surfaceData->surface);
            _device = vk::su::createDevice(_physicalDevice, _queueFamilyIndicies.first, vk::su::getDeviceExtensions());
            _commandPool = vk::su::createCommandPool(_device, _queueFamilyIndicies.first);

            _commandBuffer = std::move(_device->allocateCommandBuffersUnique(
                                           vk::CommandBufferAllocateInfo(_commandPool.get(),
                                           vk::CommandBufferLevel::ePrimary, 1)
                                           ).front());

            _graphicsQueue = _device->getQueue(_queueFamilyIndicies.first, 0);
            _presentQueue = _device->getQueue(_queueFamilyIndicies.second, 0);

            _swapChainData = new vk::su::SwapChainData(_physicalDevice,
                                                _device,
                                                *(_surfaceData->surface),
                                                _surfaceData->extent,
                                                vk::ImageUsageFlagBits::eColorAttachment
                                                    | vk::ImageUsageFlagBits::eTransferSrc,
                                                vk::UniqueSwapchainKHR(),
                                                _queueFamilyIndicies.first,
                                                _queueFamilyIndicies.second);


            _depthBufferData = new vk::su::DepthBufferData(_physicalDevice,
                                                    _device,
                                                    vk::Format::eD16Unorm,
                                                    _surfaceData->extent);

            _uniformBufferData = new vk::su::BufferData(_physicalDevice,
                                                 _device,
                                                 sizeof(glm::mat4x4),
                                                 vk::BufferUsageFlagBits::eUniformBuffer);
            vk::su::copyToDevice(_device,
                                 _uniformBufferData->deviceMemory,
                                 // TODO: Custom MVP
                                 vk::su::createModelViewProjectionClipMatrix(_surfaceData->extent));

            _descriptorSetLayout = vk::su::createDescriptorSetLayout(_device,
                                                                    {{
                                                                    vk::DescriptorType::eUniformBuffer,
                                                                    1,
                                                                    vk::ShaderStageFlagBits::eVertex
                                                                    }});

            _pipelineLayout = _device->createPipelineLayoutUnique(
                        vk::PipelineLayoutCreateInfo(
                            vk::PipelineLayoutCreateFlags(),
                            1,
                            &_descriptorSetLayout.get()
                            )
                        );


            _renderPass = vk::su::createRenderPass(_device,
                                                   vk::su::pickSurfaceFormat(
                                                       _physicalDevice.getSurfaceFormatsKHR(
                                                           _surfaceData->surface.get()
                                                           )
                                                       ).format,
                                                   _depthBufferData->format
                                                   );

            _vertexShaderModule = ngfx::loadShader("shaders/vert.spv",_device);
            _fragmentShaderModule = ngfx::loadShader("shaders/frag.spv",_device);

            _framebuffers = vk::su::createFramebuffers(_device,
                                                     _renderPass,
                                                     _swapChainData->imageViews,
                                                     _depthBufferData->imageView,
                                                     _surfaceData->extent);

            _vertexBufferData = new vk::su::BufferData(_physicalDevice,
                                                _device,
                                                geometrySize,
                                                vk::BufferUsageFlagBits::eVertexBuffer);

            vk::su::copyToDevice(_device,
                                 _vertexBufferData->deviceMemory,
                                 geometry,
                                 geometrySize / sizeof(geometry[0]));

            _descriptorPool =
                    vk::su::createDescriptorPool(_device, { {vk::DescriptorType::eUniformBuffer, 1} });

            _descriptorSet = std::move(_device->allocateDescriptorSetsUnique(
                                           vk::DescriptorSetAllocateInfo(
                                               *_descriptorPool,
                                               1,
                                               &*_descriptorSetLayout)
                                           ).front()
                                       );

            vk::su::updateDescriptorSets(_device,
                                         _descriptorSet,
                                         {{
                                            vk::DescriptorType::eUniformBuffer,
                                            _uniformBufferData->buffer,
                                            vk::UniqueBufferView()
                                         }},
                                         {});
            _pipelineCache = _device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
            _graphicsPipeline = vk::su::createGraphicsPipeline(_device,
                                                               _pipelineCache,
                                                               std::make_pair(*_vertexShaderModule, nullptr),
                                                               std::make_pair(*_fragmentShaderModule, nullptr),
                                                               sizeof(geometry[0]),
                                                               {{vk::Format::eR32G32B32A32Sfloat, 0 },
                                                                {vk::Format::eR32G32B32A32Sfloat, 16 }
                                                               },
                                                               vk::FrontFace::eClockwise,
                                                               true,
                                                               _pipelineLayout,
                                                               _renderPass);
        }

        void draw(void)
        {
            // Get the index of the next available swapchain image:
            vk::UniqueSemaphore imageAcquiredSemaphore = _device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
            vk::ResultValue<uint32_t> currentBuffer = _device->acquireNextImageKHR(_swapChainData->swapChain.get(), vk::su::FenceTimeout, imageAcquiredSemaphore.get(), nullptr);
            assert(currentBuffer.result == vk::Result::eSuccess);
            assert(currentBuffer.value < _framebuffers.size());

            _commandBuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));

            vk::ClearValue clearValues[2];
            clearValues[0].color = vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
            clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
            vk::RenderPassBeginInfo renderPassBeginInfo(_renderPass.get(),
                                                        _framebuffers[currentBuffer.value].get(),
                                                        vk::Rect2D(vk::Offset2D(0, 0),
                                                                   _surfaceData->extent),
                                                        2,
                                                        clearValues);
            _commandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
            _commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline.get());
            _commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout.get(),
                                               0,
                                               _descriptorSet.get(),
                                               nullptr);

            _commandBuffer->bindVertexBuffers(0, *(_vertexBufferData->buffer), {0});
            _commandBuffer->setViewport(0, vk::Viewport(0.0f,
                                                        0.0f,
                                                        static_cast<float>(_surfaceData->extent.width),
                                                        static_cast<float>(_surfaceData->extent.height),
                                                        0.0f,
                                                        1.0f));
            _commandBuffer->setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _surfaceData->extent));

            _commandBuffer->draw(12 * 3, 1, 0, 0);
            _commandBuffer->endRenderPass();
            _commandBuffer->end();

            vk::UniqueFence drawFence = _device->createFenceUnique(vk::FenceCreateInfo());

            vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
            vk::SubmitInfo submitInfo(1, &imageAcquiredSemaphore.get(), &waitDestinationStageMask, 1, &_commandBuffer.get());
            _graphicsQueue.submit(submitInfo, drawFence.get());

            while (vk::Result::eTimeout == _device->waitForFences(drawFence.get(), VK_TRUE, vk::su::FenceTimeout))
              ;

            _presentQueue.presentKHR(vk::PresentInfoKHR(0, nullptr, 1, &_swapChainData->swapChain.get(), &currentBuffer.value));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            /* VULKAN_KEY_END */

            _device->waitIdle();
        }

        
    private:
        vk::UniqueInstance _instance;
        vk::UniqueDebugUtilsMessengerEXT _debugUtilsMessenger;
        vk::PhysicalDevice _physicalDevice;
        std::pair<uint32_t, uint32_t> _queueFamilyIndicies; // {Graphics, Present}
        vk::UniqueDevice _device;
        vk::UniqueCommandPool _commandPool;

        // TODO: allocate Data classes better
        vk::su::SwapChainData *_swapChainData;
        vk::su::SurfaceData *_surfaceData;
        vk::su::DepthBufferData *_depthBufferData;
        vk::su::BufferData *_uniformBufferData;
        vk::su::BufferData *_vertexBufferData;

        vk::UniqueCommandBuffer _commandBuffer;
        vk::Queue _graphicsQueue;
        vk::Queue _presentQueue;
        vk::UniqueDescriptorSetLayout _descriptorSetLayout;
        vk::UniqueRenderPass _renderPass;
        vk::UniqueShaderModule _vertexShaderModule;
        vk::UniqueShaderModule _fragmentShaderModule;
        std::vector<vk::UniqueFramebuffer> _framebuffers;
        vk::UniqueDescriptorPool _descriptorPool;
        vk::UniqueDescriptorSet _descriptorSet;
        vk::UniquePipelineLayout _pipelineLayout;
        vk::UniquePipelineCache _pipelineCache;
        vk::UniquePipeline _graphicsPipeline;
    };



}

int main(int /*argc*/, char ** /*argv*/)
{
    ngfx::Renderer gfx(ngfx::kTestData, sizeof(ngfx::kTestData));
    while (1) gfx.draw();
}

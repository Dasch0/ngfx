#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "util.hpp"

namespace ngfx
{
  namespace util
  {
    void buildLayout(
        vk::Device *device,
        size_t descLayoutCount,
        vk::DescriptorSetLayout *descLayouts,
        size_t pushSize,
        vk::PipelineLayout *pipelineLayout)
    {
      vk::PushConstantRange pushConstants[] = {
        vk::PushConstantRange(
            vk::ShaderStageFlagBits::eVertex,
            0,
            pushSize),
      };

      vk::PipelineLayoutCreateInfo pipelineLayoutCI(
          vk::PipelineLayoutCreateFlags(),
          descLayoutCount,
          descLayouts,
          util::array_size(pushConstants),
          pushConstants);

      device->createPipelineLayout(
          &pipelineLayoutCI,
          nullptr,
          pipelineLayout);
    }

    // TODO: dynamic state on viewport/scissor to speed up resize
    // Should implement working resize code first
    // TODO: Investigate whether there is any performance benefit to
    // using derivatives with any vendor, for now just using a cache
    void buildPipeline(
        vk::Device *device,
        vk::Extent2D extent,
        vk::VertexInputBindingDescription *bindingDesc,
        size_t bindingSize,
        vk::VertexInputAttributeDescription *attributeDesc,
        size_t attributeSize,
        const std::string &vertPath,
        const std::string &fragPath,
        bool overlay, //TODO: Fix hacky way of passing primative type to pipeline
        vk::PipelineLayout *pipelineLayout,
        vk::RenderPass *renderPass,
        vk::PipelineCache *cache,
        vk::Pipeline *pipeline) 
  {
      auto vertShaderCode = util::readFile(vertPath);
      auto fragShaderCode = util::readFile(fragPath);

      vk::ShaderModule vertModule = 
        util::createShaderModule(device, vertShaderCode);
       vk::ShaderModule fragModule =
        util::createShaderModule(device, fragShaderCode);

      vk::PipelineShaderStageCreateInfo vertStageCI(
          vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eVertex,
          vertModule,
          "main");

      vk::PipelineShaderStageCreateInfo fragStageCI(
          vk::PipelineShaderStageCreateFlags(),
          vk::ShaderStageFlagBits::eFragment,
          fragModule,
          "main");

      vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertStageCI,
        fragStageCI
      };

      vk::PipelineVertexInputStateCreateInfo vertexInputCI(
          vk::PipelineVertexInputStateCreateFlags(),
          bindingSize,
          bindingDesc,
          attributeSize,
          attributeDesc);

      vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
          vk::PipelineInputAssemblyStateCreateFlags(),
          overlay ? vk::PrimitiveTopology::eTriangleList
          : vk::PrimitiveTopology::eLineStrip,
          false);

      vk::Viewport viewport(
          0.0f,
          0.0f,
          extent.width,
          extent.height,
          0.0,
          1.0);

      vk::Rect2D scissor(vk::Offset2D(0, 0), extent);

      vk::PipelineViewportStateCreateInfo viewportCI(
          vk::PipelineViewportStateCreateFlags(),
          1,
          &viewport,
          1,
          &scissor);

      vk::PipelineRasterizationStateCreateInfo rasterizerCI(
          vk::PipelineRasterizationStateCreateFlags(),
          false,
          false,
          vk::PolygonMode::eFill,
          vk::CullModeFlagBits::eBack,
          overlay ? vk::FrontFace::eClockwise
          : vk::FrontFace::eCounterClockwise,
          false,
          0.0f,
          0.0f,
          0.0f,
          1.0f);

      vk::PipelineMultisampleStateCreateInfo multisamplingCI(
          vk::PipelineMultisampleStateCreateFlags(),
          vk::SampleCountFlagBits::e1,
          false,
          1.0f,
          nullptr,
          false,
          false);

      vk::PipelineColorBlendAttachmentState colorBlendAttachment(
          false,
          vk::BlendFactor::eOne,
          vk::BlendFactor::eZero,
          vk::BlendOp::eAdd,
          vk::BlendFactor::eOne,
          vk::BlendFactor::eZero,
          vk::BlendOp::eAdd,
          vk::ColorComponentFlagBits::eR
          | vk::ColorComponentFlagBits::eG
          | vk::ColorComponentFlagBits::eB
          | vk::ColorComponentFlagBits::eA);

      vk::PipelineColorBlendStateCreateInfo colorBlendingCI(
          vk::PipelineColorBlendStateCreateFlags(),
          false,
          vk::LogicOp::eCopy,
          1,
          &colorBlendAttachment);

      vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
      };

      vk::PipelineDynamicStateCreateInfo dynamicStateCI(
          vk::PipelineDynamicStateCreateFlags(),
          2,
          dynamicStates);

      vk::GraphicsPipelineCreateInfo pipelineCI(
          vk::PipelineCreateFlags(),
          2,
          shaderStages,
          &vertexInputCI,
          &inputAssembly,
          nullptr,
          &viewportCI,
          &rasterizerCI,
          &multisamplingCI,
          nullptr,
          &colorBlendingCI,
          nullptr,
          *pipelineLayout,
          *renderPass,
          0,
          nullptr,
          -1);

      device->createGraphicsPipelines(
          *cache,
          1,
          &pipelineCI,
          nullptr,
          pipeline);

      device->destroyShaderModule(vertModule);
      device->destroyShaderModule(fragModule);
    }
  }
}

#ifndef NGFX_PIPELINE_H
#define NGFX_PIPELINE_H

#include "vulkan/vulkan.hpp"

namespace ngfx
{
  namespace util
  {
    void buildLayout(
        vk::Device *device,
        size_t pushSize,
        vk::PipelineLayout *pipelineLayout);

    void buildPipeline(
        vk::Device *device,
        vk::Extent2D extent,
        vk::VertexInputBindingDescription *bindingDesc,
        size_t bindingSize,
        vk::VertexInputAttributeDescription *attributeDesc,
        size_t attributeSize,
        const std::string &vertPath,
        const std::string &fragPath,
        vk::PipelineLayout *pipelineLayout,
        vk::RenderPass *renderPass,
        vk::PipelineCache *cache,
        vk::Pipeline *pipeline);
  }
}

#endif //NGFX_PIPELINE_H

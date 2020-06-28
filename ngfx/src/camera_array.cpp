#include "camera_array.hpp"
#include "vulkan/vulkan.hpp"
#include "context.hpp"
#include "util.hpp"
#include "pipeline.hpp"

namespace ngfx
{
  vk::VertexInputAttributeDescription CameraArray::attribute[] = {
    // Per Vertex data
    vk::VertexInputAttributeDescription(
        0,
        0,
        vk::Format::eR32G32Sfloat,
        offsetof(util::Vertex, pos)),
    vk::VertexInputAttributeDescription(
        1,
        0,
        vk::Format::eR32G32B32Sfloat,
        offsetof(util::Vertex, color)),
    vk::VertexInputAttributeDescription(
        2,
        0,
        vk::Format::eR32G32Sfloat,
        offsetof(util::Vertex, texCoord)),
    // Per Instance Data
    vk::VertexInputAttributeDescription(
        3,
        1,
        vk::Format::eR32G32Sfloat,
        offsetof(util::Instance, pos)),
  };

  vk::VertexInputBindingDescription CameraArray::binding[] = {
    vk::VertexInputBindingDescription(
        0,
        sizeof(util::Vertex),
        vk::VertexInputRate::eVertex),
    vk::VertexInputBindingDescription(
        1,
        sizeof(util::Instance),
        vk::VertexInputRate::eInstance)
  };

  CameraArray::CameraArray(Context *c)
    : device(&c->device)
  {
    buildRenderPass();
    buildFbo(c);
    util::buildLayout(
        device,
        0,
        nullptr,
        sizeof(mvp),
        &layout);
    
    util::buildPipeline(
        &c->device,
        fbo.extent,
        binding,
        util::array_size(binding),
        attribute,
        util::array_size(attribute),
        "shaders/env_vert.spv",
        "shaders/env_frag.spv",
        false,
        &layout,
        &pass,
        &c->pipelineCache,
        &pipeline);
  }

  void CameraArray::buildRenderPass()
  {
    vk::AttachmentDescription colorAttachment(
        vk::AttachmentDescriptionFlags(),
        vk::Format::eR8G8B8A8Srgb,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal);
    
    vk::AttachmentReference colorAttachmentRef(
        0,
        vk::ImageLayout::eColorAttachmentOptimal);
    
    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &colorAttachmentRef,
        nullptr,
        nullptr,
        0,
        nullptr);
    
    vk::SubpassDependency subpassDependency(
        0,
        VK_SUBPASS_EXTERNAL,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlags());

    vk::RenderPassCreateInfo renderPassCI(
        vk::RenderPassCreateFlags(),
        1,
        &colorAttachment,
        1,
        &subpass,
        1,
        &subpassDependency);
    
    device->createRenderPass(
        &renderPassCI,
        nullptr,
        &pass);
  }

  void CameraArray::buildFbo(Context *c)
  {
    uint w = 256;
    uint h = 256;
    vk::Image *i = &fbo.image;
    vk::DeviceMemory *m = &fbo.mem;
    vk::ImageView *v = &fbo.view;
    vk::Framebuffer *f = &fbo.frame;

    fbo.extent = vk::Extent2D(w, h);
    
    vk::ImageCreateInfo imageCI(
        vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(w, h, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment
        | vk::ImageUsageFlagBits::eSampled
        | vk::ImageUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive);
    
    device->createImage(&imageCI, nullptr, i);

    vk::MemoryRequirements memReqs;
    device->getImageMemoryRequirements(*i, &memReqs);
    
    auto memType = util::findMemoryType(
        c->physicalDevice,
        memReqs.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    vk::MemoryAllocateInfo allocInfo(
        memReqs.size,
        memType);
    
    c->device.allocateMemory(&allocInfo, nullptr, m);
    c->device.bindImageMemory(*i, *m, 0);

    vk::ImageViewCreateInfo viewCI(
        vk::ImageViewCreateFlags(),
        *i,
        vk::ImageViewType::e2D,
        vk::Format::eR8G8B8A8Srgb,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        0, //baseMipLevel
        1, //levelCount
        0, //baseArrayLayer
        1)); //layerCount

    c->device.createImageView(&viewCI, nullptr, v);

    // Framebuffer
    vk::FramebufferCreateInfo framebufferCI(
        vk::FramebufferCreateFlags(),
        pass,
        1,
        v,
        w,
        h,
        1); // Layer
    
    c->device.createFramebuffer(
        &framebufferCI,
        nullptr,
        f);
  }

  CameraArray::~CameraArray()
  {
    device->destroyRenderPass(pass);
  }
}

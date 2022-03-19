#include "Render.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

void Render::createDepthStencilImage()
{
	depthStencilImage.create(*vmaAllocator, width, height, 1, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	vk::ImageViewCreateInfo viewCreateInfo = {
	.image = vk::Image(depthStencilImage.image),
	.viewType = vk::ImageViewType::e2D,
	.format = vk::Format::eD24UnormS8Uint,
	.components = {.r = vk::ComponentSwizzle::eR,
					.g = vk::ComponentSwizzle::eG,
					.b = vk::ComponentSwizzle::eB,
					.a = vk::ComponentSwizzle::eA},
	.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1},
	};
	depthStencilImageView = device->createImageViewUnique(viewCreateInfo);
}

void Render::createRenderPass()
{
	vk::AttachmentDescription attachments[2];
	// color
	attachments[0].format = swapchainImageFormat;
	attachments[0].samples = vk::SampleCountFlagBits::e1;;
	attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
	attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout = vk::ImageLayout::eUndefined;
	attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;
	// depth
	attachments[1].format = vk::Format::eD24UnormS8Uint;
	attachments[1].samples = vk::SampleCountFlagBits::e1;;
	attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
	attachments[1].storeOp = vk::AttachmentStoreOp::eStore;
	attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[1].initialLayout = vk::ImageLayout::eUndefined;
	attachments[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference color_reference = {
		.attachment = 0,
		.layout = vk::ImageLayout::eColorAttachmentOptimal };

	vk::AttachmentReference depth_reference = {
		.attachment = 1,
		.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal };

	vk::SubpassDescription subpass = {
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_reference,
		.pDepthStencilAttachment = &depth_reference };

	// Subpass dependency to wait for wsi image acquired semaphore before starting layout transition
	vk::SubpassDependency subpassDenpendency = {
	.srcSubpass = VK_SUBPASS_EXTERNAL,
	.dstSubpass = 0,
	.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
	.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
	.srcAccessMask = {},
	.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite };

	vk::RenderPassCreateInfo renderPassCreateInfo{ .attachmentCount = 2,
												.pAttachments = attachments,
												.subpassCount = 1,
												.pSubpasses = &subpass,
												.dependencyCount = 1,
												.pDependencies = &subpassDenpendency };

	renderPass = device->createRenderPassUnique(renderPassCreateInfo);

	// create gui renderpass
	attachments[0].loadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[1].loadOp = vk::AttachmentLoadOp::eDontCare;
	guiRenderPass = device->createRenderPassUnique(renderPassCreateInfo);
}

void Render::createFrameBuffers()
{
	LOGI("create framebuffer");

	vk::ImageView attachments[2];
	attachments[1] = depthStencilImageView.get();

	vk::FramebufferCreateInfo framebufferCreateInfo = {
	.renderPass = renderPass.get(),
	.attachmentCount = 2,
	.pAttachments = attachments,
	.width = width,
	.height = height,
	.layers = 1 };

	framebuffers.resize(swapchainImageViews.size());
	for (unsigned i = 0; i < swapchainImageViews.size(); ++i)
	{
		attachments[0] = swapchainImageViews[i].get();
		framebuffers[i] = device->createFramebufferUnique(framebufferCreateInfo);
	}
}


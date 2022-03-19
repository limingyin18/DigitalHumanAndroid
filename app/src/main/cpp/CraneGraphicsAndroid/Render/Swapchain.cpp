#include "Render.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

void Render::createSwapchain()
{
	LOGI("create swapchain");

	vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface.get());
	vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface.get());
	swapchainImageFormat = formats[0].format;

	vk::SurfaceCapabilitiesKHR surfCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
	width = surfCapabilities.currentExtent.width;
	height = surfCapabilities.currentExtent.height;


	// ѡ�����ģʽ��prefer > Mailbox > FIFORelaxed > FIFO

	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	if (any_of(presentModes.cbegin(), presentModes.cend(), [this](vk::PresentModeKHR p)
		{ return p == preferPresentMode; }))
		presentMode = preferPresentMode;
	else if (any_of(presentModes.cbegin(), presentModes.cend(), [](vk::PresentModeKHR p)
		{ return p == vk::PresentModeKHR::eMailbox; }))
		presentMode = vk::PresentModeKHR::eMailbox;
	else if (any_of(presentModes.cbegin(), presentModes.cend(), [](vk::PresentModeKHR p)
		{ return p == vk::PresentModeKHR::eFifoRelaxed; }))
		presentMode = vk::PresentModeKHR::eFifoRelaxed;

	// create swapchain

	// ָ������������Ϣ
	vk::SwapchainCreateInfoKHR swapchainCreateInfo{ .surface = surface.get(),
													.minImageCount = surfCapabilities.minImageCount + 1,
													.imageFormat = swapchainImageFormat,
													.imageColorSpace = formats[0].colorSpace,
													.imageExtent = {.width = width, .height = height},
													.imageArrayLayers = 1,
													.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
													.imageSharingMode = vk::SharingMode::eExclusive,
													.preTransform = surfCapabilities.currentTransform,
													.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
													.presentMode = presentMode,
													.clipped = true,
													.oldSwapchain = nullptr };
	unsigned queueFamilyIndices[2] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };
	if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
	{
		// If the graphics and present queues are from different queue families,
		// we either have to explicitly transfer ownership of images between the
		// queues, or we have to create the swapchain with imageSharingMode
		// as VK_SHARING_MODE_CONCURRENT
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapchain = device->createSwapchainKHRUnique(swapchainCreateInfo);

	// get swapchain images
	swapchainImages = device->getSwapchainImagesKHR(swapchain.get());

	// create swapchain image views
	swapchainImageViews.resize(swapchainImages.size());
	for (uint32_t i = 0; i < swapchainImageViews.size(); i++)
	{
		vk::ImageViewCreateInfo viewInfo{
		.image = swapchainImages[i],
		.viewType = vk::ImageViewType::e2D,
		.format = swapchainCreateInfo.imageFormat,
		.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1} };

		swapchainImageViews[i] = device->createImageViewUnique(viewInfo);
	}
}
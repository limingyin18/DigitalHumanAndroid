#include "Render.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

void Render::createCommandPool()
{
	LOGI("创建命令池");

	vk::CommandPoolCreateInfo commandPoolCreateInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
	.queueFamilyIndex = graphicsQueueFamilyIndex };
	commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);

	vk::CommandPoolCreateInfo commandPoolCreateInfoCompute{ .queueFamilyIndex = computeQueueFamilyIndex };
	commandPoolCompute = device->createCommandPoolUnique(commandPoolCreateInfoCompute);
}

void Render::allocateCommandBuffer()
{
	LOGI("分配命令缓冲");

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo{ .commandPool = commandPool.get(),
		.level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = static_cast<uint32_t>(swapchainImages.size()) };
	commandBuffer = device->allocateCommandBuffersUnique(commandBufferAllocateInfo);

	vk::CommandBufferAllocateInfo commandBufferAllocateInfoCompute{ .commandPool = commandPoolCompute.get(),
		.level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
	commandBuffersCompute = device->allocateCommandBuffersUnique(commandBufferAllocateInfoCompute);
}

void Crane::Render::createSynchronization()
{
	LOGI("创建同步");

	// create semaphores
	imageAcquiredSemaphores.resize(swapchainImages.size());
	renderFinishedSemaphores.resize(swapchainImages.size());
	guiRenderFinishedSemaphores.resize(swapchainImages.size());
	for (size_t i = 0; i < swapchainImages.size(); ++i)
	{
		// create image acquired semaphore
		vk::SemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo{};
		imageAcquiredSemaphores[i] = device->createSemaphoreUnique(imageAcquiredSemaphoreCreateInfo);

		// create render finished semaphore
		vk::SemaphoreCreateInfo renderFinishedSemaphoreCreateInfo{};
		renderFinishedSemaphores[i] = device->createSemaphoreUnique(renderFinishedSemaphoreCreateInfo);

		// create gui render finished semaphore
		vk::SemaphoreCreateInfo guiRenderFinishedSemaphoreCreateInfo{};
		guiRenderFinishedSemaphores[i] = device->createSemaphoreUnique(guiRenderFinishedSemaphoreCreateInfo);
	}

	// create fence
	inFlightFences.resize(swapchainImages.size());
	imagesInFlightFences.resize(swapchainImages.size());
	for (size_t i = 0; i < swapchainImages.size(); ++i)
	{
		vk::FenceCreateInfo inFlightFenceInfo{ .flags = vk::FenceCreateFlagBits::eSignaled };
		inFlightFences[i] = device->createFenceUnique(inFlightFenceInfo);
		imagesInFlightFences[i] = inFlightFences[i].get();
	}

	// set submit info
	waitPipelineStageFlags = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = waitPipelineStageFlags.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.signalSemaphoreCount = 1;

	// present info
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.get();
	presentInfo.waitSemaphoreCount = 1;

	// set cull submit info
	submitInfoCompute.commandBufferCount = commandBuffersCompute.size();
	submitInfoCompute.pCommandBuffers = &commandBuffersCompute[0].get();
}


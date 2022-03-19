#include "Render.hpp"

using namespace std;
using namespace Eigen;
using namespace Crane;

vk::CommandBuffer Render::beginSingleTimeCommands()
{
	vk::CommandBufferAllocateInfo commandBufferAllocateInfo{ .commandPool = commandPool.get(),
		.level = vk::CommandBufferLevel::ePrimary,  .commandBufferCount = 1 };

	auto singleBuffer = device->allocateCommandBuffers(commandBufferAllocateInfo);

	vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
	singleBuffer[0].begin(beginInfo);

	return singleBuffer[0];
}

void Render::endSingleTimeCommands(vk::CommandBuffer cmdBuffer)
{
	cmdBuffer.end();

	vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &cmdBuffer };

	graphicsQueue.submit(1, &submitInfo, vk::Fence{});
	graphicsQueue.waitIdle();

	device->freeCommandBuffers(commandPool.get(), 1, &cmdBuffer);
}

void Render::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(VkCommandBuffer(commandBuffer), srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

size_t Render::padUniformBufferSize(size_t originalSize, VkPhysicalDeviceProperties gpuProperties)
{
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = gpuProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}

uint32_t Crane::Render::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice(physicalDevice), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

std::tuple<Image, vk::UniqueImageView> Render::createTextureImage(uint32_t texWidth, uint32_t texHeight, uint32_t texChannels, void* pixels)
{
	Image image;
	image.create(*vmaAllocator, texWidth, texHeight, texChannels,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);
	vk::CommandBuffer cmdBuff = beginSingleTimeCommands();
	Buffer stagingBuffer = Buffer::createStagingBuffer(pixels, image.imageSize, *vmaAllocator);
	image.updateBuffer(VkBuffer(stagingBuffer.buffer), cmdBuff);
	endSingleTimeCommands(cmdBuff);

	vk::ImageViewCreateInfo imageViewCreateInfo{ .image = vk::Image(image.image),
												.viewType = vk::ImageViewType::e2D,
												.format = vk::Format::eR8G8B8A8Unorm,
												.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
																	 .baseMipLevel = 0,
																	 .levelCount = 1,
																	 .baseArrayLayer = 0,
																	 .layerCount = 1} };

	vk::UniqueImageView imageView = device->createImageViewUnique(imageViewCreateInfo);
	return { std::move(image), std::move(imageView) };
}

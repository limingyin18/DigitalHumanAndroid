#include "Image.hpp"

using namespace std;
using namespace Crane;

Image::~Image()
{
	if (image != VK_NULL_HANDLE)
	{
		vmaDestroyImage(vmaAllocator, image, imageMemory);
		image = VK_NULL_HANDLE;
		imageMemory = VK_NULL_HANDLE;
		imageSize = 0;
	}
}

Image::Image(Image&& rhs)
{
	vmaAllocator = rhs.vmaAllocator;
	image = rhs.image;
	imageMemory = rhs.imageMemory;
	extent = rhs.extent;
	imageSize = rhs.imageSize;

	rhs.image = VK_NULL_HANDLE;
}

Image& Image::operator=(Image&& rhs)
{
	if (this != &rhs)
	{
		vmaAllocator = rhs.vmaAllocator;
		image = rhs.image;
		imageMemory = rhs.imageMemory;
		extent = rhs.extent;
		imageSize = rhs.imageSize;

		rhs.image = VK_NULL_HANDLE;
	}

	return *this;
}

void Image::create(VmaAllocator alloc, uint32_t width, uint32_t height, uint32_t channels,
	VkFormat format,
	VkImageTiling tiling, VkImageUsageFlags usage,
	VmaMemoryUsage vmaUsage)
{
	imageSize = width * height * channels;
	vmaAllocator = alloc;
	extent.width = width;
	extent.height = height;
	extent.depth = 1;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = extent;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = vmaUsage;
	auto res = vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo, &image, &imageMemory, nullptr);
	if (res != VK_SUCCESS)
		throw std::runtime_error("failed to create image!");
}

void Image::update(const void* srcData, vk::CommandBuffer cmdBuff)
{
	Buffer stagingBuffer = Buffer::createStagingBuffer(srcData, imageSize, vmaAllocator);

	transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuff);

	copyBufferToImage(VkBuffer(stagingBuffer.buffer), cmdBuff);

	transitionImageLayout(image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuff);
}

void Image::updateBuffer(VkBuffer buffer, vk::CommandBuffer cmdBuff)
{
	transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdBuff);
	copyBufferToImage(buffer, cmdBuff);
	transitionImageLayout(image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuff);
}

void Image::copyBufferToImage(VkBuffer buffer, vk::CommandBuffer commandBuffer)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = extent;

	vkCmdCopyBufferToImage(VkCommandBuffer(commandBuffer), buffer, image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void Image::transitionImageLayout(const VkImage &image,
								  VkImageLayout oldLayout, VkImageLayout newLayout,
								  vk::CommandBuffer commandBuffer)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(VkCommandBuffer(commandBuffer), sourceStage, destinationStage, 0, 0, nullptr, 0,
		nullptr, 1, &barrier);
}

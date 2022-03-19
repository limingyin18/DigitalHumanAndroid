#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include "Buffer.hpp"

namespace Crane
{
    /**
     * @brief ��װVkImage��VmaAllocator
    */
    class Image
    {
    public:
        explicit Image() = default;
        ~Image();

        Image(Image &&rhs);
        Image(const Image &rhs) = delete;
        Image &operator=(const Image &rhs) = delete;
        Image &operator=(Image &&rhs);

        void create(VmaAllocator alloc, uint32_t width, uint32_t height, uint32_t channels,
                    VkFormat format,
                    VkImageTiling tiling, VkImageUsageFlags usage,
                    VmaMemoryUsage vmaUsage);
        void update(const void *srcData, vk::CommandBuffer cmdBuff);

        void updateBuffer(VkBuffer buffer, vk::CommandBuffer cmdBuff);

        void copyBufferToImage(VkBuffer buffer, vk::CommandBuffer commandBuffer);
        static void transitionImageLayout(const VkImage &image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                          vk::CommandBuffer commandBuffer);

    public:
        VmaAllocator vmaAllocator = VK_NULL_HANDLE;
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation imageMemory = VK_NULL_HANDLE;
        VkExtent3D extent;
        VkDeviceSize imageSize;
    };
}
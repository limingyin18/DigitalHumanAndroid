#pragma once

#include <cstring>
#include <vector>
#include <stdexcept>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_SETTERS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace Crane
{
    class Buffer
    {
    public:
        /**
         * @brief Creates a buffer using VMA
         */
        explicit Buffer() = default;

        ~Buffer();

        Buffer(const Buffer& rhs) = delete;

        Buffer(Buffer&& rhs) noexcept;

        Buffer& operator=(const Buffer& rhs) = delete;

        Buffer& operator=(Buffer&& rhs) = delete;

        void create(VmaAllocator alloc, VkDeviceSize size, VkBufferUsageFlags usage,
            VmaMemoryUsage vmaUsage, VmaAllocationCreateFlagBits allocationCreateFlagBits = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            const std::vector<uint32_t> pQueueFamilyIndices = std::vector<uint32_t>());

        void update(const void* srcData);

        static Buffer createStagingBuffer(const void* srcData, VkDeviceSize size,
            VmaAllocator vmaAllocator);

        template<class T>
        static void createVmaBufferFromVector(const std::vector<T>& dataVector,
            VmaAllocator vmaAllocator,
            Buffer& buffer, VkBufferUsageFlags usage,
            VmaMemoryUsage vmaUsage,
            VkCommandBuffer cmdBuff)
        {
            VkDeviceSize bufferSize = sizeof(T) * dataVector.size();
            Buffer stagingBuffer = Buffer::createStagingBuffer(dataVector.data(), bufferSize,
                vmaAllocator);

            buffer.create(vmaAllocator, bufferSize, usage, vmaUsage);

            VkBufferCopy copyRegion{};
            copyRegion.size = bufferSize;
            vkCmdCopyBuffer(cmdBuff, VkBuffer(stagingBuffer.buffer), VkBuffer(buffer.buffer), 1, &copyRegion);
        };

    public:
        VmaAllocator vmaAllocator = VK_NULL_HANDLE;
        uint32_t size = 0;
        vk::Buffer buffer;
        VmaAllocation bufferMemory = VK_NULL_HANDLE;

        vk::DescriptorBufferInfo descriptorBufferInfo;
    };
}
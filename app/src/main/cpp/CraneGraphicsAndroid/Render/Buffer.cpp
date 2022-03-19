#include "Buffer.hpp"

using namespace std;
using namespace Crane;

Buffer::~Buffer()
{
    if (vmaAllocator != nullptr)
        vmaDestroyBuffer(vmaAllocator, VkBuffer(buffer), bufferMemory);
    buffer = nullptr;
    bufferMemory = VK_NULL_HANDLE;
}

Buffer::Buffer(Buffer&& rhs) noexcept
{
    vmaAllocator = rhs.vmaAllocator;
    buffer = rhs.buffer;
    bufferMemory = rhs.bufferMemory;
    size = rhs.size;
    descriptorBufferInfo = rhs.descriptorBufferInfo;

    rhs.vmaAllocator = nullptr;
    rhs.buffer = nullptr;
    rhs.bufferMemory = VK_NULL_HANDLE;
    rhs.size = 0;
}

void Buffer::create(VmaAllocator alloc, VkDeviceSize size, VkBufferUsageFlags usage,
    VmaMemoryUsage vmaUsage, VmaAllocationCreateFlagBits allocationCreateFlagBits,
    VkSharingMode sharingMode, const vector<uint32_t> pQueueFamilyIndices)
{
    vmaAllocator = alloc;
    this->size = size;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = sharingMode;
    bufferInfo.queueFamilyIndexCount = pQueueFamilyIndices.size();
    bufferInfo.pQueueFamilyIndices = pQueueFamilyIndices.data();

    VmaAllocationCreateInfo vmaAllocCreateInfo{};
    vmaAllocCreateInfo.usage = vmaUsage;
    vmaAllocCreateInfo.flags = allocationCreateFlagBits;
    VmaAllocationInfo vmaAllocInfo{};
    VkBuffer* b = reinterpret_cast<VkBuffer*>(&buffer);
    if (vmaCreateBuffer(vmaAllocator, &bufferInfo, &vmaAllocCreateInfo, b, &bufferMemory,
        &vmaAllocInfo) != VK_SUCCESS)
        throw std::runtime_error("failed to create vma buffer");

    descriptorBufferInfo.buffer = vk::Buffer(buffer);
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = size;
}

void Buffer::update(const void* srcData)
{
    VmaAllocationInfo allocInfo;
    vmaGetAllocationInfo(vmaAllocator, bufferMemory, &allocInfo);
    memcpy(allocInfo.pMappedData, srcData, allocInfo.size);
}

Buffer Buffer::createStagingBuffer(const void* srcData, VkDeviceSize size,
    VmaAllocator vmaAllocator)
{
    Buffer stagingBuffer;
    stagingBuffer.create(vmaAllocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);
    stagingBuffer.update(srcData);

    return stagingBuffer;
}

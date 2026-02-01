#include "Buffer.h"
#include "AppState.h"
#include "MemoryAllocateInfo.h"
#include "Utils.h"

void Buffer::Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage)
{
	this->size = size;

	VkBufferCreateInfo bufferInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo allocInfo { };
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	CHECK_VKCMD(vmaCreateBuffer(appState.Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));
}

void Buffer::CreateStagingBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
}

void Buffer::CreateHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void Buffer::CreateHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void Buffer::CreateHostVisibleIndexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void Buffer::CreateHostVisibleUniformBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

void Buffer::Map(AppState& appState)
{
	CHECK_VKCMD(vmaMapMemory(appState.Allocator, allocation, &mapped));
}

void Buffer::UnmapAndFlush(AppState &appState) const
{
	vmaUnmapMemory(appState.Allocator, allocation);
	CHECK_VKCMD(vmaFlushAllocation(appState.Allocator, allocation, 0, VK_WHOLE_SIZE));
}

void Buffer::Delete(AppState& appState) const
{
	vmaDestroyBuffer(appState.Allocator, buffer, allocation);
}

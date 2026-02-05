#include "Buffer.h"
#include "AppState.h"
#include "MemoryAllocateInfo.h"
#include "Utils.h"

void Buffer::Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, bool mappable)
{
	this->size = size;

	VkBufferCreateInfo bufferInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo allocInfo { };
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	if (mappable) allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	CHECK_VKCMD(vmaCreateBuffer(appState.Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));
}

void Buffer::CreateStagingBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
}

void Buffer::CreateVertexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
}

void Buffer::CreateMappableVertexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
}

void Buffer::CreateIndexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
}

void Buffer::CreateMappableIndexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true);
}

void Buffer::CreateMappableUniformBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
}

void Buffer::CreateStorageBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
}

void Buffer::CreateMappableStorageBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
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

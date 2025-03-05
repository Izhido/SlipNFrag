#include "Buffer.h"
#include "AppState.h"
#include "MemoryAllocateInfo.h"
#include "Utils.h"

void Buffer::Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	this->size = size;

	VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(appState.Device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, properties, memoryAllocateInfo, true);

	CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &memory));

	CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, memory, 0));
}

void Buffer::CreateHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Buffer::CreateHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Buffer::CreateHostVisibleUniformBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Buffer::CreateSourceBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}

void Buffer::Delete(AppState& appState) const
{
	if (mapped != nullptr)
	{
		vkUnmapMemory(appState.Device, memory);
	}
	if (buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(appState.Device, buffer, nullptr);
	}
	if (memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(appState.Device, memory, nullptr);
	}
}

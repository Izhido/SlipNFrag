#include "SharedMemoryBuffer.h"
#include "AppState.h"
#include "MemoryAllocateInfo.h"
#include "Utils.h"
#include "Constants.h"

void SharedMemoryBuffer::Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	this->size = size;
	VkBufferCreateInfo bufferCreateInfo { };
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &buffer));
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(appState.Device, buffer, &memoryRequirements);
	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, properties, memoryAllocateInfo);
	VkDeviceSize alignmentCorrection = 0;
	VkDeviceSize alignmentExcess = appState.Scene.usedInLatestBufferSharedMemory % memoryRequirements.alignment;
	if (alignmentExcess > 0)
	{
		alignmentCorrection = memoryRequirements.alignment - alignmentExcess;
	}
	if (appState.Scene.latestBufferSharedMemory == nullptr || appState.Scene.usedInLatestBufferSharedMemory + alignmentCorrection + memoryAllocateInfo.allocationSize > MEMORY_BLOCK_SIZE)
	{
		VkDeviceSize size = MEMORY_BLOCK_SIZE;
		if (size < memoryAllocateInfo.allocationSize)
		{
			size = memoryAllocateInfo.allocationSize;
		}
		auto memoryBlockAllocateInfo = memoryAllocateInfo;
		memoryBlockAllocateInfo.allocationSize = size;
		appState.Scene.latestBufferSharedMemory = new SharedMemory { };
		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryBlockAllocateInfo, nullptr, &appState.Scene.latestBufferSharedMemory->memory));
		appState.Scene.usedInLatestBufferSharedMemory = 0;
		alignmentCorrection = 0;
	}
	CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, appState.Scene.latestBufferSharedMemory->memory, appState.Scene.usedInLatestBufferSharedMemory + alignmentCorrection));
	appState.Scene.usedInLatestBufferSharedMemory += (alignmentCorrection + memoryAllocateInfo.allocationSize);
	appState.Scene.latestBufferSharedMemory->referenceCount++;
	sharedMemory = appState.Scene.latestBufferSharedMemory;
}

void SharedMemoryBuffer::CreateVertexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void SharedMemoryBuffer::CreateIndexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void SharedMemoryBuffer::Delete(AppState& appState)
{
	if (buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(appState.Device, buffer, nullptr);
	}
	sharedMemory->referenceCount--;
	if (sharedMemory->referenceCount == 0)
	{
		vkFreeMemory(appState.Device, sharedMemory->memory, nullptr);
		delete sharedMemory;
	}
}

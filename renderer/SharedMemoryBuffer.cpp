#include "SharedMemoryBuffer.h"
#include "AppState.h"
#include "MemoryAllocateInfo.h"
#include "Utils.h"
#include "Constants.h"

void SharedMemoryBuffer::Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	this->size = size;
	this->properties = properties;

	VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(appState.Device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo { };
	updateMemoryAllocateInfo(appState, memoryRequirements, this->properties, memoryAllocateInfo, true);

	auto create = true;
	auto& latestMemory = appState.Scene.latestMemory[memoryAllocateInfo.memoryTypeIndex];
	for (auto entry = latestMemory.begin(); entry != latestMemory.end(); entry++)
	{
		VkDeviceSize alignmentCorrection = 0;
		VkDeviceSize alignmentExcess = entry->used % memoryRequirements.alignment;
		if (alignmentExcess > 0)
		{
			alignmentCorrection = memoryRequirements.alignment - alignmentExcess;
		}
		if (entry->used + alignmentCorrection + memoryAllocateInfo.allocationSize <= entry->memory->size)
		{
			create = false;
			sharedMemory = entry->memory;
			CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, sharedMemory->memory, entry->used + alignmentCorrection));
			entry->used += (alignmentCorrection + memoryAllocateInfo.allocationSize);
			if (entry->used >= entry->memory->size)
			{
				sharedMemory->referenceCount--;
				latestMemory.erase(entry);
			}
			break;
		}
	}

	if (create)
	{
		sharedMemory = new SharedMemory { };
		sharedMemory->size = Constants::memoryBlockSize;
		auto add = true;
		if (sharedMemory->size < memoryAllocateInfo.allocationSize)
		{
			sharedMemory->size = memoryAllocateInfo.allocationSize;
			add = false;
		}
		auto memoryBlockAllocateInfo = memoryAllocateInfo;
		memoryBlockAllocateInfo.allocationSize = sharedMemory->size;
		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryBlockAllocateInfo, nullptr, &sharedMemory->memory));
		if (add)
		{
			latestMemory.push_back({ sharedMemory, memoryAllocateInfo.allocationSize });
			sharedMemory->referenceCount++;
		}
		CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, sharedMemory->memory, 0));
	}

	sharedMemory->referenceCount++;
}

void SharedMemoryBuffer::CreateVertexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void SharedMemoryBuffer::CreateIndexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void SharedMemoryBuffer::CreateStorageBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void SharedMemoryBuffer::Delete(AppState& appState) const
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

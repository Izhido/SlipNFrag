#include "CachedBuffers.h"
#include "Constants.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"

#define MINIMUM_ALLOCATION 4096

Buffer* CachedBuffers::Get(AppState& appState, VkDeviceSize size)
{
	for (Buffer** b = &oldBuffers; *b != nullptr; b = &(*b)->next)
	{
		if ((*b)->size >= size && (*b)->size < size * 2)
		{
			auto buffer = *b;
			*b = (*b)->next;
			return buffer;
		}
	}
	return nullptr;
}

VkDeviceSize CachedBuffers::MinimumAllocationFor(VkDeviceSize size)
{
	VkDeviceSize result = size * 5 / 4;
	if (result < MINIMUM_ALLOCATION)
	{
		result = MINIMUM_ALLOCATION;
	}
	return result;
}

Buffer* CachedBuffers::GetVertexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(appState, size);
	if (buffer == nullptr)
	{
		buffer = new Buffer();
		buffer->CreateHostVisibleVertexBuffer(appState, MinimumAllocationFor(size));
		VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped));
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetIndexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(appState, size);
	if (buffer == nullptr)
	{
		buffer = new Buffer();
		buffer->CreateIndexBuffer(appState, MinimumAllocationFor(size));
		VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped));
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetStagingBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(appState, size);
	if (buffer == nullptr)
	{
		buffer = new Buffer();
		buffer->CreateStagingBuffer(appState, MinimumAllocationFor(size));
		VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped));
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetStagingStorageBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(appState, size);
	if (buffer == nullptr)
	{
		buffer = new Buffer();
		buffer->CreateStagingStorageBuffer(appState, MinimumAllocationFor(size));
		VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped));
	}
	MoveToFront(buffer);
	return buffer;
}

void CachedBuffers::DeleteOld(AppState& appState)
{
	for (Buffer** b = &oldBuffers; *b != nullptr; )
	{
		(*b)->unusedCount++;
		if ((*b)->unusedCount >= MAX_UNUSED_COUNT)
		{
			Buffer* next = (*b)->next;
			(*b)->Destroy(appState);
			delete *b;
			*b = next;
		}
		else
		{
			b = &(*b)->next;
		}
	}
}

void CachedBuffers::DisposeFront()
{
	for (Buffer* b = buffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		b->next = oldBuffers;
		oldBuffers = b;
	}
	buffers = nullptr;
}

void CachedBuffers::Reset(AppState& appState)
{
	DeleteOld(appState);
	DisposeFront();
}

void CachedBuffers::MoveToFront(Buffer* buffer)
{
	buffer->unusedCount = 0;
	buffer->next = buffers;
	buffers = buffer;
}

void CachedBuffers::Delete(AppState& appState)
{
	for (Buffer* b = buffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		b->Destroy(appState);
		delete b;
	}
	for (Buffer* b = oldBuffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		b->Destroy(appState);
		delete b;
	}
}

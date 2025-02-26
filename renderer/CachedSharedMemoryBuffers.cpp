#include "CachedSharedMemoryBuffers.h"
#include "Constants.h"
#include "AppState.h"
#include "Utils.h"

SharedMemoryBuffer* CachedSharedMemoryBuffers::Get(AppState& appState, VkDeviceSize size)
{
	for (SharedMemoryBuffer** b = &oldBuffers; *b != nullptr; b = &(*b)->next)
	{
		if ((((*b)->size == Constants::minimumBufferAllocation &&
			  size <= Constants::minimumBufferAllocation)) ||
			((*b)->size >= size && (*b)->size < size * 2))
		{
			auto buffer = *b;
			(*b) = (*b)->next;
			return buffer;
		}
	}
	return nullptr;
}

VkDeviceSize CachedSharedMemoryBuffers::MinimumAllocationFor(VkDeviceSize size)
{
	VkDeviceSize result = size * 5 / 4;
	if (result < Constants::minimumBufferAllocation)
	{
		result = Constants::minimumBufferAllocation;
	}
	return result;
}

SharedMemoryBuffer* CachedSharedMemoryBuffers::GetVertexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(appState, size);
	if (buffer == nullptr)
	{
		buffer = new SharedMemoryBuffer { };
		buffer->CreateVertexBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

SharedMemoryBuffer* CachedSharedMemoryBuffers::GetIndexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(appState, size);
	if (buffer == nullptr)
	{
		buffer = new SharedMemoryBuffer { };
		buffer->CreateIndexBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

SharedMemoryBuffer* CachedSharedMemoryBuffers::GetStorageBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(appState, size);
	if (buffer == nullptr)
	{
		buffer = new SharedMemoryBuffer { };
		buffer->CreateStorageBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

void CachedSharedMemoryBuffers::DeleteOld(AppState& appState)
{
	for (SharedMemoryBuffer** b = &oldBuffers; *b != nullptr; )
	{
		(*b)->unusedCount++;
		if ((*b)->unusedCount >= Constants::maxUnusedCount)
		{
			auto next = (*b)->next;
			(*b)->Delete(appState);
			delete *b;
			(*b) = next;
		}
		else
		{
			b = &(*b)->next;
		}
	}
}

void CachedSharedMemoryBuffers::DisposeFront()
{
	for (SharedMemoryBuffer* b = buffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->next = oldBuffers;
		oldBuffers = b;
	}
	buffers = nullptr;
}

void CachedSharedMemoryBuffers::Reset(AppState& appState)
{
	DeleteOld(appState);
	DisposeFront();
}

void CachedSharedMemoryBuffers::MoveToFront(SharedMemoryBuffer* buffer)
{
	buffer->unusedCount = 0;
	buffer->next = buffers;
	buffers = buffer;
}

void CachedSharedMemoryBuffers::Delete(AppState& appState) const
{
	for (SharedMemoryBuffer* b = buffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
	for (SharedMemoryBuffer* b = oldBuffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
}

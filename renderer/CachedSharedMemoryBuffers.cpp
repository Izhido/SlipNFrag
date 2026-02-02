#include "CachedSharedMemoryBuffers.h"
#include "Constants.h"
#include "AppState.h"
#include "Utils.h"

SharedMemoryBuffer* CachedSharedMemoryBuffers::Get(VkDeviceSize size)
{
	for (auto b = oldBuffers.begin(); b != oldBuffers.end(); b++)
	{
		if ((((*b)->size == Constants::minimumBufferAllocation &&
			  size <= Constants::minimumBufferAllocation)) ||
			((*b)->size >= size && (*b)->size < size * 2))
		{
			auto buffer = *b;
			oldBuffers.erase(b);
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

SharedMemoryBuffer* CachedSharedMemoryBuffers::GetIndexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = new SharedMemoryBuffer { };
		buffer->CreateIndexBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

void CachedSharedMemoryBuffers::Reset(AppState& appState)
{
	for (auto b = oldBuffers.begin(); b != oldBuffers.end(); )
	{
		(*b)->unusedCount++;
		if ((*b)->unusedCount >= Constants::maxUnusedCount)
		{
			auto buffer = *b;
			buffer->Delete(appState);
			delete buffer;
			b = oldBuffers.erase(b);
		}
		else
		{
			b++;
		}
	}
	oldBuffers.splice(oldBuffers.begin(), buffers);
}

void CachedSharedMemoryBuffers::MoveToFront(SharedMemoryBuffer* buffer)
{
	buffer->unusedCount = 0;
	buffers.push_back(buffer);
}

void CachedSharedMemoryBuffers::Delete(AppState& appState)
{
	for (auto b : oldBuffers)
	{
		b->Delete(appState);
	}
	oldBuffers.clear();
	for (auto b : buffers)
	{
		b->Delete(appState);
	}
	buffers.clear();
}

#include "CachedBuffers.h"
#include "Constants.h"
#include "AppState.h"
#include "Utils.h"

Buffer* CachedBuffers::Get(VkDeviceSize size)
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

VkDeviceSize CachedBuffers::MinimumAllocationFor(VkDeviceSize size)
{
	VkDeviceSize result = size * 5 / 4;
	if (result < Constants::minimumBufferAllocation)
	{
		result = Constants::minimumBufferAllocation;
	}
	return result;
}

Buffer* CachedBuffers::GetHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = new Buffer { };
		buffer->CreateHostVisibleVertexBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = new Buffer { };
		buffer->CreateHostVisibleStorageBuffer(appState, size);
	}
	MoveToFront(buffer);
	return buffer;
}

void CachedBuffers::Reset(AppState& appState)
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

void CachedBuffers::MoveToFront(Buffer* buffer)
{
	buffer->unusedCount = 0;
	buffers.push_back(buffer);
}

void CachedBuffers::Delete(AppState& appState)
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

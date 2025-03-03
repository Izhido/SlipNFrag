#include "CachedBuffers.h"
#include "Constants.h"
#include "AppState.h"
#include "Utils.h"

Buffer* CachedBuffers::Get(AppState& appState, VkDeviceSize size)
{
	for (Buffer** b = &oldBuffers; *b != nullptr; b = &(*b)->next)
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
	auto buffer = Get(appState, size);
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
	auto buffer = Get(appState, size);
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
	for (Buffer** b = &oldBuffers; *b != nullptr; )
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
	for (Buffer* b = buffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->next = oldBuffers;
		oldBuffers = b;
	}
	buffers = nullptr;
}

void CachedBuffers::MoveToFront(Buffer* buffer)
{
	buffer->unusedCount = 0;
	buffer->next = buffers;
	buffers = buffer;
}

void CachedBuffers::Delete(AppState& appState) const
{
	for (Buffer* b = buffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
	for (Buffer* b = oldBuffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
}

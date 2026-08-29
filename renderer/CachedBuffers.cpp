#include "CachedBuffers.h"
#include "Constants.h"
#include "AppState.h"
#include "Utils.h"

Buffer* CachedBuffers::Acquire()
{
	if (reusable != nullptr)
	{
		auto reused = reusable;
		reusable = reusable->next;
		memset(reused, 0, sizeof(Buffer));
		return reused;
	}
	return new Buffer { };
}

Buffer* CachedBuffers::Get(VkDeviceSize size)
{
	if (toDispose != nullptr)
	{
		for (auto b = &toDispose; *b != nullptr; b = &(*b)->next)
		{
			if ((((*b)->size == Constants::minimumBufferAllocation &&
				size <= Constants::minimumBufferAllocation)) ||
				((*b)->size >= size && (*b)->size < size * 2))
			{
				auto buffer = *b;
				(*b) = buffer->next;
				buffer->next = nullptr;
 				return buffer;
			}
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

Buffer* CachedBuffers::GetStagingBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = Acquire();
		buffer->CreateStagingBuffer(appState, size);
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetVertexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = Acquire();
		buffer->CreateVertexBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetMappableVertexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = Acquire();
		buffer->CreateMappableVertexBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetIndexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = Acquire();
		buffer->CreateIndexBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetMappableIndexBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = Acquire();
		buffer->CreateMappableIndexBuffer(appState, MinimumAllocationFor(size));
	}
	MoveToFront(buffer);
	return buffer;
}

Buffer* CachedBuffers::GetMappableStorageBuffer(AppState& appState, VkDeviceSize size)
{
	auto buffer = Get(size);
	if (buffer == nullptr)
	{
		buffer = Acquire();
		buffer->CreateMappableStorageBuffer(appState, size);
	}
	MoveToFront(buffer);
	return buffer;
}

void CachedBuffers::Reset(AppState& appState)
{
	if (toDispose != nullptr)
	{
		for (auto b = &toDispose; *b != nullptr; )
		{
			(*b)->unusedCount++;
			if ((*b)->unusedCount >= Constants::framesToLive)
			{
				auto buffer = *b;
				buffer->Delete(appState);
				(*b) = buffer->next;
				buffer->next = reusable;
				reusable = buffer;
			}
			else
			{
				b = &(*b)->next;
			}
		}
	}
	if (current != nullptr)
	{
		current->next = toDispose;
		toDispose = current;
		current = nullptr;
	}
}

void CachedBuffers::MoveToFront(Buffer* buffer)
{
	buffer->unusedCount = 0;
	current = buffer;
}

void CachedBuffers::Delete(AppState& appState)
{
	for (Buffer* b = reusable, *next; b != nullptr; b = next)
	{
		next = b->next;
		delete b;
	}
	reusable = nullptr;
	for (Buffer* b = toDispose, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
	toDispose = nullptr;
	if (current != nullptr)
	{
		current->Delete(appState);
		delete current;
		current = nullptr;
	}
}

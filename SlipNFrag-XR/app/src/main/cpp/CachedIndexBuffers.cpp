#include "CachedIndexBuffers.h"
#include "Constants.h"

void CachedIndexBuffers::Initialize()
{
	firstAliasIndices8 = nullptr;
	currentAliasIndices8 = nullptr;
	firstAliasIndices16 = nullptr;
	currentAliasIndices16 = nullptr;
	firstAliasIndices32 = nullptr;
	currentAliasIndices32 = nullptr;
}

void CachedIndexBuffers::SetupAliasIndices8(LoadedIndexBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentAliasIndices8 == nullptr)
	{
		firstAliasIndices8 = &loaded;
	}
	else
	{
		currentAliasIndices8->next = &loaded;
	}
	currentAliasIndices8 = &loaded;
}

void CachedIndexBuffers::SetupAliasIndices16(LoadedIndexBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentAliasIndices16 == nullptr)
	{
		firstAliasIndices16 = &loaded;
	}
	else
	{
		currentAliasIndices16->next = &loaded;
	}
	currentAliasIndices16 = &loaded;
}

void CachedIndexBuffers::SetupAliasIndices32(LoadedIndexBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentAliasIndices32 == nullptr)
	{
		firstAliasIndices32 = &loaded;
	}
	else
	{
		currentAliasIndices32->next = &loaded;
	}
	currentAliasIndices32 = &loaded;
}

void CachedIndexBuffers::DisposeFront()
{
	for (SharedMemoryBuffer* b = buffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->next = oldBuffers;
		oldBuffers = b;
	}
	buffers = nullptr;
}

void CachedIndexBuffers::MoveToFront(SharedMemoryBuffer* buffer)
{
	buffer->unusedCount = 0;
	buffer->next = buffers;
	buffers = buffer;
}

void CachedIndexBuffers::Delete(AppState& appState) const
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

void CachedIndexBuffers::DeleteOld(AppState& appState)
{
	if (oldBuffers != nullptr)
	{
		for (auto b = &oldBuffers; *b != nullptr; )
		{
			(*b)->unusedCount++;
			if ((*b)->unusedCount >= Constants::maxUnusedCount)
			{
				auto next = (*b)->next;
				(*b)->Delete(appState);
				delete *b;
				*b = next;
			}
			else
			{
				b = &(*b)->next;
			}
		}
	}
}

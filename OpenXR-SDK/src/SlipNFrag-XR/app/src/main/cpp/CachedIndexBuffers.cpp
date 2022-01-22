#include "CachedIndexBuffers.h"
#include "Constants.h"

void CachedIndexBuffers::Initialize()
{
	firstIndices8 = nullptr;
	currentIndices8 = nullptr;
	firstIndices16 = nullptr;
	currentIndices16 = nullptr;
	firstIndices32 = nullptr;
	currentIndices32 = nullptr;
	firstAliasIndices8 = nullptr;
	currentAliasIndices8 = nullptr;
	firstAliasIndices16 = nullptr;
	currentAliasIndices16 = nullptr;
	firstAliasIndices32 = nullptr;
	currentAliasIndices32 = nullptr;
}

void CachedIndexBuffers::SetupIndices8(LoadedIndexBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentIndices8 == nullptr)
	{
		firstIndices8 = &loaded;
	}
	else
	{
		currentIndices8->next = &loaded;
	}
	currentIndices8 = &loaded;
}

void CachedIndexBuffers::SetupIndices16(LoadedIndexBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentIndices16 == nullptr)
	{
		firstIndices16 = &loaded;
	}
	else
	{
		currentIndices16->next = &loaded;
	}
	currentIndices16 = &loaded;
}

void CachedIndexBuffers::SetupIndices32(LoadedIndexBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentIndices32 == nullptr)
	{
		firstIndices32 = &loaded;
	}
	else
	{
		currentIndices32->next = &loaded;
	}
	currentIndices32 = &loaded;
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
	for (Buffer* b = buffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->next = oldBuffers;
		oldBuffers = b;
	}
	buffers = nullptr;
}

void CachedIndexBuffers::MoveToFront(Buffer* buffer)
{
	buffer->unusedCount = 0;
	buffer->next = buffers;
	buffers = buffer;
}

void CachedIndexBuffers::Delete(AppState& appState) const
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

#include "CachedAliasBuffers.h"
#include "Constants.h"

void CachedAliasBuffers::Initialize()
{
	firstAliasVertices = nullptr;
	currentAliasVertices = nullptr;
	firstAliasTexCoords = nullptr;
	currentAliasTexCoords = nullptr;
}

void CachedAliasBuffers::SetupAliasVertices(LoadedSharedMemoryBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentAliasVertices == nullptr)
	{
		firstAliasVertices = &loaded;
	}
	else
	{
		currentAliasVertices->next = &loaded;
	}
	currentAliasVertices = &loaded;
}

void CachedAliasBuffers::SetupAliasTexCoords(LoadedSharedMemoryTexCoordsBuffer &loaded)
{
	loaded.next = nullptr;
	if (currentAliasTexCoords == nullptr)
	{
		firstAliasTexCoords = &loaded;
	}
	else
	{
		currentAliasTexCoords->next = &loaded;
	}
	currentAliasTexCoords = &loaded;
}

void CachedAliasBuffers::DisposeFront()
{
	for (Buffer* b = buffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->next = oldBuffers;
		oldBuffers = b;
	}
	buffers = nullptr;
}

void CachedAliasBuffers::MoveToFront(Buffer* buffer)
{
	buffer->unusedCount = 0;
	buffer->next = buffers;
	buffers = buffer;
}

void CachedAliasBuffers::Delete(AppState& appState)
{
	for (Buffer* b = buffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
	buffers = nullptr;
	for (Buffer* b = oldBuffers, *next; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
	oldBuffers = nullptr;
}

void CachedAliasBuffers::DeleteOld(AppState& appState)
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
				(*b) = next;
			}
			else
			{
				b = &(*b)->next;
			}
		}
	}
}

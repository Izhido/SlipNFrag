#include "CachedSharedMemoryBuffers.h"
#include "Constants.h"

void CachedSharedMemoryBuffers::SetupVertices(LoadedSharedMemoryBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentVertices == nullptr)
	{
		firstVertices = &loaded;
	}
	else
	{
		currentVertices->next = &loaded;
	}
	currentVertices = &loaded;
}

void CachedSharedMemoryBuffers::SetupIndices16(LoadedSharedMemoryIndexBuffer& loaded)
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

void CachedSharedMemoryBuffers::SetupIndices32(LoadedSharedMemoryIndexBuffer& loaded)
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

void CachedSharedMemoryBuffers::SetupSurfaceTexturePosition(LoadedSharedMemoryBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentSurfaceTexturePosition == nullptr)
	{
		firstSurfaceTexturePosition = &loaded;
	}
	else
	{
		currentSurfaceTexturePosition->next = &loaded;
	}
	currentSurfaceTexturePosition = &loaded;
}

void CachedSharedMemoryBuffers::SetupTurbulentTexturePosition(LoadedSharedMemoryBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentTurbulentTexturePosition == nullptr)
	{
		firstTurbulentTexturePosition = &loaded;
	}
	else
	{
		currentTurbulentTexturePosition->next = &loaded;
	}
	currentTurbulentTexturePosition = &loaded;
}

void CachedSharedMemoryBuffers::SetupAliasVertices(LoadedSharedMemoryBuffer& loaded)
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

void CachedSharedMemoryBuffers::SetupAliasTexCoords(LoadedSharedMemoryTexCoordsBuffer &loaded)
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

void CachedSharedMemoryBuffers::DisposeFront()
{
	for (SharedMemoryBuffer* b = buffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		b->next = oldBuffers;
		oldBuffers = b;
	}
	buffers = nullptr;
}

void CachedSharedMemoryBuffers::MoveToFront(SharedMemoryBuffer* buffer)
{
	buffer->unusedCount = 0;
	buffer->next = buffers;
	buffers = buffer;
}

void CachedSharedMemoryBuffers::Delete(AppState& appState)
{
	for (SharedMemoryBuffer* b = buffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
	for (SharedMemoryBuffer* b = oldBuffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		b->Delete(appState);
		delete b;
	}
}

void CachedSharedMemoryBuffers::DeleteOld(AppState& appState)
{
	if (oldBuffers != nullptr)
	{
		for (auto b = &oldBuffers; *b != nullptr; )
		{
			(*b)->unusedCount++;
			if ((*b)->unusedCount >= MAX_UNUSED_COUNT)
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

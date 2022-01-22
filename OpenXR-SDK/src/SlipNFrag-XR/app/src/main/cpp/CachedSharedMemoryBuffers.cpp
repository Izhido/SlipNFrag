#include "CachedSharedMemoryBuffers.h"
#include "Constants.h"

void CachedSharedMemoryBuffers::Initialize()
{
	firstVertices = nullptr;
	currentVertices = nullptr;
	firstSurfaceTexturePositions = nullptr;
	currentSurfaceTexturePositions = nullptr;
	firstTurbulentTexturePositions = nullptr;
	currentTurbulentTexturePositions = nullptr;
	firstAliasVertices = nullptr;
	currentAliasVertices = nullptr;
	firstAliasTexCoords = nullptr;
	currentAliasTexCoords = nullptr;
}

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

void CachedSharedMemoryBuffers::SetupSurfaceTexturePositions(LoadedSharedMemoryTexturePositionsBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentSurfaceTexturePositions == nullptr)
	{
		firstSurfaceTexturePositions = &loaded;
	}
	else
	{
		currentSurfaceTexturePositions->next = &loaded;
	}
	currentSurfaceTexturePositions = &loaded;
}

void CachedSharedMemoryBuffers::SetupTurbulentTexturePositions(LoadedSharedMemoryTexturePositionsBuffer& loaded)
{
	loaded.next = nullptr;
	if (currentTurbulentTexturePositions == nullptr)
	{
		firstTurbulentTexturePositions = &loaded;
	}
	else
	{
		currentTurbulentTexturePositions->next = &loaded;
	}
	currentTurbulentTexturePositions = &loaded;
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
	for (SharedMemoryBuffer* b = buffers, *next; b != nullptr; b = next)
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

void CachedSharedMemoryBuffers::DeleteOld(AppState& appState)
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

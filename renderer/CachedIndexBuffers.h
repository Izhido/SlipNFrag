#pragma once

#include "LoadedIndexBuffer.h"

struct CachedIndexBuffers
{
	SharedMemoryBuffer* buffers;
	SharedMemoryBuffer* oldBuffers;
	LoadedIndexBuffer* firstAliasIndices8;
	LoadedIndexBuffer* currentAliasIndices8;
	LoadedIndexBuffer* firstAliasIndices16;
	LoadedIndexBuffer* currentAliasIndices16;
	LoadedIndexBuffer* firstAliasIndices32;
	LoadedIndexBuffer* currentAliasIndices32;

	void Initialize();
	void SetupAliasIndices8(LoadedIndexBuffer& loaded);
	void SetupAliasIndices16(LoadedIndexBuffer& loaded);
	void SetupAliasIndices32(LoadedIndexBuffer& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};

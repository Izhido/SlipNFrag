#pragma once

#include "LoadedIndexBuffer.h"

struct CachedIndexBuffers
{
	Buffer* buffers;
	Buffer* oldBuffers;
	LoadedIndexBuffer* firstIndices8;
	LoadedIndexBuffer* currentIndices8;
	LoadedIndexBuffer* firstIndices16;
	LoadedIndexBuffer* currentIndices16;
	LoadedIndexBuffer* firstIndices32;
	LoadedIndexBuffer* currentIndices32;
	LoadedIndexBuffer* firstAliasIndices8;
	LoadedIndexBuffer* currentAliasIndices8;
	LoadedIndexBuffer* firstAliasIndices16;
	LoadedIndexBuffer* currentAliasIndices16;
	LoadedIndexBuffer* firstAliasIndices32;
	LoadedIndexBuffer* currentAliasIndices32;

	void Initialize();
	void SetupIndices8(LoadedIndexBuffer& loaded);
	void SetupIndices16(LoadedIndexBuffer& loaded);
	void SetupIndices32(LoadedIndexBuffer& loaded);
	void SetupAliasIndices8(LoadedIndexBuffer& loaded);
	void SetupAliasIndices16(LoadedIndexBuffer& loaded);
	void SetupAliasIndices32(LoadedIndexBuffer& loaded);
	void DisposeFront();
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState) const;
	void DeleteOld(AppState& appState);
};

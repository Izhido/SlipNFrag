#pragma once

#include "Buffer.h"

struct CachedBuffers
{
	Buffer* buffers = nullptr;
	Buffer* oldBuffers = nullptr;

	void DeleteOld(AppState& appState);
	void DisposeFront();
	void Reset(AppState& appState);
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState);
};

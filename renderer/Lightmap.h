#pragma once

#include <list>
#include "LightmapBuffer.h"

struct AppState;

struct Lightmap
{
	int createdFrameCount;
	int unusedCount;
	int width;
	int height;
	LightmapBuffer* buffer;
	int allocatedIndex;
	VkDeviceSize offset;
	bool filled;

	void Create(AppState& appState, uint32_t width, uint32_t height);
	void Delete(AppState& appState) const;
};

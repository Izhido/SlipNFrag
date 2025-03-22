#pragma once

#include <list>
#include "LightmapBuffer.h"

struct AppState;

struct Lightmap
{
	Lightmap* next;
	int createdFrameCount;
	int unusedCount;
	int width;
	int height;
	LightmapBuffer* buffer;
	VkDeviceSize offset;

	void Create(AppState& appState, uint32_t width, uint32_t height);
	void Delete(AppState& appState) const;
};

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
	void* texture;
	LightmapBuffer* buffer;
	VkDeviceSize offset;

	bool Create(AppState& appState, uint32_t width, uint32_t height, void* texture);
	void Delete(AppState& appState) const;
};

#pragma once

#include <list>
#include "LightmapBuffer.h"

struct Lightmap
{
	Lightmap* next;
	int createdFrameCount;
	int unusedCount;
	int width;
	int height;
	bool variable;
	LightmapBuffer* buffer;
	VkDeviceSize offset;

	bool Create(AppState& appState, uint32_t width, uint32_t height, bool variable);
	void Delete(AppState& appState) const;
};

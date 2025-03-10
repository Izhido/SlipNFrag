#pragma once

#include <list>
#include "LightmapRGBBuffer.h"

struct AppState;

struct LightmapRGB
{
	int createdFrameCount;
	int unusedCount;
	int width;
	int height;
	LightmapRGBBuffer* buffer;
	int allocatedIndex;
	VkDeviceSize offset;
	bool filled;

	void Create(AppState& appState, uint32_t width, uint32_t height);
	void Delete(AppState& appState) const;
};

#pragma once

#include <list>
#include "LightmapRGBBuffer.h"

struct AppState;

struct LightmapRGB
{
	LightmapRGB* next;
	int createdFrameCount;
	int unusedCount;
	int width;
	int height;
	void* texture;
	LightmapRGBBuffer* buffer;
	VkDeviceSize offset;

	bool Create(AppState& appState, uint32_t width, uint32_t height, void* texture);
	void Delete(AppState& appState) const;
};

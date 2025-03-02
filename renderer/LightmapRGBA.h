#pragma once

#include <list>
#include "LightmapRGBATexture.h"

struct AppState;

struct LightmapRGBA
{
	LightmapRGBA* next;
	int createdFrameCount;
	int unusedCount;
	int width;
	int height;
	LightmapRGBATexture* texture;
	int allocatedIndex;
	VkDeviceSize offset;
	bool filled;

	void Create(AppState& appState, uint32_t width, uint32_t height);
	void Delete(AppState& appState) const;
};

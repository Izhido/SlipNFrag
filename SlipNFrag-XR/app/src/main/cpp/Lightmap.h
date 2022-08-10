#pragma once

#include <list>
#include "LightmapTexture.h"

struct AppState;

struct Lightmap
{
	Lightmap* next;
	int createdFrameCount;
	int unusedCount;
	int width;
	int height;
	std::list<LightmapTexture>* textureList;
	std::list<LightmapTexture>::iterator texture;
	int allocatedIndex;
	bool filled;

	void Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
	void Delete(AppState& appState) const;
};

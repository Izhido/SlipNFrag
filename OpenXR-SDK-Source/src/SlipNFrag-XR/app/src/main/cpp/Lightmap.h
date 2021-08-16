#pragma once

#include "TwinKey.h"
#include "AllocationList.h"
#include "Buffer.h"
#include "StagingBuffer.h"

struct Lightmap
{
	Lightmap* next;
	TwinKey key;
	int unusedCount;
	int width;
	int height;
	int layerCount;
	VkImage image;
	AllocationList* allocationList;
	std::list<Allocation>::iterator allocation;
	int allocatedIndex;
	VkImageView view;
	VkDescriptorSet descriptorSet;
	bool filled;

	void Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
	void Fill(AppState& appState, StagingBuffer& buffer);
	void Delete(AppState& appState) const;
};

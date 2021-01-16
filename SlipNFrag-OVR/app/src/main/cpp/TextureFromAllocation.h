#pragma once

struct TextureFromAllocation
{
	TextureFromAllocation* next;
	TwinKey key;
	int unusedCount;
	int width;
	int height;
	int mipCount;
	int layerCount;
	VkImage image;
	AllocationList* allocationList;
	int allocationIndex;
	int allocatedIndex;
	VkImageView view;
};

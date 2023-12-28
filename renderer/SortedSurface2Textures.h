#pragma once

#include <vector>

struct SortedSurface2Textures
{
	VkDescriptorSet texture;
	VkDescriptorSet glowTexture;
	std::vector<int> entries;
	VkDeviceSize indexCount;
};

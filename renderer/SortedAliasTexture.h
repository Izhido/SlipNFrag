#pragma once

#include "SortedAliasVertices.h"

struct SortedAliasTexture
{
	VkDescriptorSet texture;
	int count;
	std::vector<SortedAliasVertices> vertices;
	std::unordered_map<VkBuffer, int> added;
};

#pragma once

#include "SortedAliasVertices.h"

struct SortedAliasTexture
{
	VkDescriptorSet texture;
	int count;
	std::vector<SortedAliasVertices> vertices;
	USE_CUSTOM_HASHMAP<VkBuffer, int> added;
};

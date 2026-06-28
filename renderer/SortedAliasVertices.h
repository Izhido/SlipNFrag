#pragma once

struct SortedAliasVertices
{
	VkBuffer vertices;
	VkBuffer texCoords;
	std::vector<int> entries;
    uint32_t firstIndex;
	uint32_t indexCount;
    uint32_t instanceCount;
};

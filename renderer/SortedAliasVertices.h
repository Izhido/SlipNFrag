#pragma once

struct SortedAliasVertices
{
	VkBuffer vertices;
	VkBuffer texCoords;
	std::vector<int> entries;
	VkDeviceSize indexCount;
};

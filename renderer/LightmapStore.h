#pragma once

#include <vector>
#include <memory>

struct LightmapStore
{
	std::vector<std::unique_ptr<uint32_t[]>> pages;
	size_t currentPageIndex;
	size_t currentPageOffset;

	uint32_t* Allocate(size_t count);
	void Clear();
};

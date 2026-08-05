#pragma once

#include <vector>
#include <memory>

struct LightmapStore
{
	std::vector<std::unique_ptr<uint32_t[]>> pages;
	size_t used;

	uint32_t* Allocate(size_t count);
	void Clear();
};

#pragma once

#include <vector>
#include <memory>

struct VertexStore
{
	std::vector<std::unique_ptr<float[]>> pages;
	size_t currentPageIndex;
	size_t currentPageOffset;

	float* Allocate(size_t count);
	void Clear();
};

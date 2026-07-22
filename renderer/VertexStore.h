#pragma once

#include <vector>
#include <memory>

class VertexStore
{
public:
	float* Allocate(size_t count);
	void Clear();

private:
	std::vector<std::unique_ptr<float[]>> pages;
	size_t currentPageIndex;
	size_t currentPageOffset;
};

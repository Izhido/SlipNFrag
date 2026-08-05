#pragma once

#include <vector>
#include <memory>

struct VertexStore
{
	std::vector<std::unique_ptr<float[]>> pages;
	size_t used;

	float* Allocate(size_t count);
	void Clear();
};

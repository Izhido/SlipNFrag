#include "VertexStore.h"
#include "Constants.h"

float* VertexStore::Allocate(size_t count)
{
	if (pages.empty() || used + count > Constants::storePageSize)
	{
		pages.push_back(std::make_unique<float[]>(Constants::storePageSize));
		used = 0;
	}

	auto result = pages.back().get() + used;
	used += count;
	return result;
}

void VertexStore::Clear()
{
	pages.clear();
	used = 0;
}

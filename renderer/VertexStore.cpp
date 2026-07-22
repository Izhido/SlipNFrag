#include "VertexStore.h"
#include "Constants.h"

float* VertexStore::Allocate(size_t count)
{
	if (pages.empty() || currentPageOffset + count > Constants::vertexStorePageSize)
	{
		if (currentPageIndex + 1 < pages.size())
		{
			currentPageIndex++;
		}
		else
		{
			pages.push_back(std::make_unique<float[]>(Constants::vertexStorePageSize));
			currentPageIndex = pages.size() - 1;
		}
		currentPageOffset = 0;
	}

	float* result = pages[currentPageIndex].get() + currentPageOffset;
	currentPageOffset += count;
	return result;
}

void VertexStore::Clear()
{
	pages.clear();
	currentPageIndex = 0;
	currentPageOffset = 0;
}

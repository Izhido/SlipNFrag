#include "LightmapStore.h"
#include "Constants.h"

uint32_t* LightmapStore::Allocate(size_t count)
{
	if (pages.empty() || currentPageOffset + count > Constants::storePageSize)
	{
		if (currentPageIndex + 1 < pages.size())
		{
			currentPageIndex++;
		}
		else
		{
			pages.push_back(std::make_unique<uint32_t[]>(Constants::storePageSize));
			currentPageIndex = pages.size() - 1;
		}
		currentPageOffset = 0;
	}

	auto result = pages[currentPageIndex].get() + currentPageOffset;
	currentPageOffset += count;
	return result;
}

void LightmapStore::Clear()
{
	pages.clear();
	currentPageIndex = 0;
	currentPageOffset = 0;
}

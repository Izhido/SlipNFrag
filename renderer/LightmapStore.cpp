#include "LightmapStore.h"
#include "Constants.h"

uint32_t* LightmapStore::Allocate(size_t count)
{
	if (pages.empty() || used + count > Constants::storePageSize)
	{
		if (count > Constants::storePageSize)
		{
			size_t roundedSize = ((count + Constants::storePageSize - 1) / Constants::storePageSize) * Constants::storePageSize;
			pages.push_back(std::make_unique<uint32_t[]>(roundedSize));
			used = Constants::storePageSize;
			return pages.back().get();
		}

		pages.push_back(std::make_unique<uint32_t[]>(Constants::storePageSize));
		used = 0;
	}

	auto result = pages.back().get() + used;
	used += count;
	return result;
}

void LightmapStore::Clear()
{
	pages.clear();
	used = 0;
}

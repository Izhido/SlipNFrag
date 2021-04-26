#pragma once

#include "Allocation.h"
#include <list>

struct AllocationList
{
	std::list<Allocation> allocations;
	VkDeviceSize blockSize = 0;
};

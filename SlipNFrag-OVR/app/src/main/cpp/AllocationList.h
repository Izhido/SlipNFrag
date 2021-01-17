#pragma once

#include "Allocation.h"

struct AllocationList
{
	std::vector<Allocation> allocations;
	VkDeviceSize blockSize = 0;
};

#pragma once

struct AllocationList
{
	std::vector<Allocation> allocations;
	VkDeviceSize blockSize;
};

#pragma once

struct Allocation
{
	VkDeviceMemory memory;
	std::vector<bool> allocated;
	int allocatedCount;
};

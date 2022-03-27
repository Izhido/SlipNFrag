#pragma once

struct SharedMemory
{
	VkDeviceMemory memory;
	VkDeviceSize size;
	int referenceCount;
};

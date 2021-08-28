#pragma once

struct SharedMemory
{
	VkDeviceMemory memory;
	int referenceCount;
};

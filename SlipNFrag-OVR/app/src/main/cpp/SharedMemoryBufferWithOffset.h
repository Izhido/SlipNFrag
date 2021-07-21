#pragma once

struct SharedMemoryBufferWithOffset
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize offset;
};

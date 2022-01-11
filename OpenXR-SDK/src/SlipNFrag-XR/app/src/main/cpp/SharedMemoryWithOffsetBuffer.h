#pragma once

struct SharedMemoryWithOffsetBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize offset;
};

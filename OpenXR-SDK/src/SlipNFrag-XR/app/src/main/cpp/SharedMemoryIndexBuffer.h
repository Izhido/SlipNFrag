#pragma once

struct SharedMemoryIndexBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize offset;
	VkIndexType indexType;
	uint32_t firstIndex;
};

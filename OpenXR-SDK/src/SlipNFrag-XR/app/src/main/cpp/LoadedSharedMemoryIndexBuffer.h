#pragma once

struct LoadedSharedMemoryIndexBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize offset;
	VkDeviceSize size;
	void* firstSource;
	void* secondSource;
	VkIndexType indexType;
	uint32_t firstIndex;
	LoadedSharedMemoryIndexBuffer* next;
};

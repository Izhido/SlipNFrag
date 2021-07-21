#pragma once

struct LoadedSharedMemoryIndexBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize offset;
	VkDeviceSize size;
	void* firstSource;
	void* secondSource;
	LoadedSharedMemoryIndexBuffer* next;
};

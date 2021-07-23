#pragma once

struct LoadedSharedMemoryAliasIndexBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize offset;
	VkDeviceSize size;
	void* source;
	LoadedSharedMemoryAliasIndexBuffer* next;
};

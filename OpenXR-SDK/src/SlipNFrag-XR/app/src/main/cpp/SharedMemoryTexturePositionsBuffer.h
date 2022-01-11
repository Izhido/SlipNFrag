#pragma once

struct SharedMemoryTexturePositionsBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize offset;
	uint32_t firstInstance;
};

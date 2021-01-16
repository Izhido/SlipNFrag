#pragma once

struct BufferWithOffset
{
	VkDeviceSize offset;
	Buffer* buffer;
};

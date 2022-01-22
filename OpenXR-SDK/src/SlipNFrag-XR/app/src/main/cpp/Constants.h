#pragma once

struct Constants
{
	static const int cpuLevel = 2;
	static const int gpuLevel = 3;
	static const VkFormat colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
	static const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	static const int screenToConsoleMultiplier = 3;
	static const int maxUnusedCount = 16;
	static const int memoryBlockSize = 1024 * 1024;
	static const int indexBuffer8BitSize = 16 * 256;
	static const int indexBuffer16BitSize = 16 * 65536;
	static const int indexBuffer32BitSize = 16 * 65536;
	static const int descriptorSetCount = 256;
};

#include "LightmapRGBATexture.h"

void LightmapRGBATexture::Initialize()
{
	width = 0;
	height = 0;
	buffer = VK_NULL_HANDLE;
	memory = VK_NULL_HANDLE;
	allocated.clear();
	allocatedCount = 0;
	firstFreeCandidate = 0;
	descriptorPool = VK_NULL_HANDLE;
	descriptorSet = VK_NULL_HANDLE;
	size = 0;
	previous = nullptr;
	next = nullptr;
}
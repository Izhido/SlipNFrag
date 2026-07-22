#pragma once

#include <vulkan/vulkan.h>
#include <cmath>

struct Constants
{
	static constexpr VkFormat colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
	static constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	static constexpr int screenToConsoleMultiplier = 3;
	static constexpr int framesToLive = 7;
	static constexpr int indexBuffer8BitSize = 16 * 256;
	static constexpr int indexBuffer16BitSize = 16 * 65536;
	static constexpr int indexBuffer32BitSize = 16 * 65536;
	static constexpr int descriptorSetCount = 256;
	static constexpr int minimumBufferAllocation = 4096;
    static constexpr int sortedSurfaceElementIncrement = 32;
	static constexpr int lightmapBufferSize = 32 * 1024;
	static constexpr int vertexStorePageSize = 256 * 1024;
	static constexpr float nearPlaneForProjection = 0.05f;
	static constexpr float farPlaneForProjection = 0;
	static constexpr float farPlaneForDepthCompositionLayer = INFINITY;
};

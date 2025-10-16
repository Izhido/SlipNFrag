#pragma once

#include <vulkan/vulkan.h>

struct Floor
{
	static VkDeviceSize VerticesSize();
	static VkDeviceSize AttributesSize();
	static VkDeviceSize IndicesSize();
	static void WriteVertices(struct AppState& appState, float* vertices);
	static void WriteAttributes(float* attributes);
	static void WriteIndices8(unsigned char* indices);
	static void WriteIndices16(uint16_t* indices);
};

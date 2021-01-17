#pragma once

#include <vulkan/vulkan.h>

struct AppState;

struct Image
{
	int width;
	int height;
	int layerCount;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;

	void Delete(AppState& appState);
};

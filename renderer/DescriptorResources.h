#pragma once

#include <vulkan/vulkan.h>

struct AppState;

struct DescriptorResources
{
	bool created;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	void Delete(AppState& appState) const;
};

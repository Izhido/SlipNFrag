#pragma once

#include <vulkan/vulkan.h>

struct DescriptorResources
{
	bool created;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	void Delete(struct AppState& appState) const;
};

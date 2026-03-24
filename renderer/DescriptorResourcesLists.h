#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct DescriptorResourcesLists
{
	bool created;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<void*> bound;

	void Delete(struct AppState& appState);
};

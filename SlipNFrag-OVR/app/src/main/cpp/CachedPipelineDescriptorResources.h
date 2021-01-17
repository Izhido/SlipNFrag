#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>

struct AppState;

struct CachedPipelineDescriptorResources
{
	bool created;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::unordered_map<void*, VkDescriptorSet> cache;
	int index;

	void Delete(AppState& appState);
};

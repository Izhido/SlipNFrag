#pragma once

#include <unordered_map>

struct CachedPipelineDescriptorResources
{
	bool created;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::unordered_map<void*, VkDescriptorSet> cache;
	int index;
};

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct AppState;

struct UpdatablePipelineDescriptorResources
{
	bool created;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<void*> bound;

	void Delete(AppState& appState);
};

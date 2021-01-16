#pragma once

struct UpdatablePipelineDescriptorResources
{
	bool created;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<void*> bound;
};

#pragma once

struct PipelineDescriptorResources
{
	bool created;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
};

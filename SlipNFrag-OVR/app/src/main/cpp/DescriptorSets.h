#pragma once

struct DescriptorSets
{
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	int used;
	int referenceCount;
};

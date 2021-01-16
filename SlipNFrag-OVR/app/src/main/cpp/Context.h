#pragma once

struct Context
{
	Device* device;
	uint32_t queueFamilyIndex;
	uint32_t queueIndex;
	VkQueue queue;
	VkCommandPool commandPool;
	VkPipelineCache pipelineCache;
};

#pragma once

#include <vector>
#include <vulkan/vulkan.h>

struct Pipeline
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	void Delete(struct AppState& appState);
};

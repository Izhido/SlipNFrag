#pragma once

#include <vector>
#include <vulkan/vulkan.h>

struct AppState;

struct Pipeline
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	void Delete(AppState& appState);
};

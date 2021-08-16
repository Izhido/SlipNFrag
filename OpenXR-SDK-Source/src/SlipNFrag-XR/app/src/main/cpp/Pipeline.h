#pragma once

#include <vector>
#include <vulkan/vulkan.h>

struct AppState;

struct Pipeline
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	void Delete(AppState& appState) const;
};

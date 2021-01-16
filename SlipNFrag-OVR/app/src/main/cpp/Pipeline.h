#pragma once

struct Pipeline
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
};

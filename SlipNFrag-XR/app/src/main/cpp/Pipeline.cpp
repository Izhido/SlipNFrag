#include "Pipeline.h"
#include "AppState.h"

void Pipeline::Delete(AppState& appState)
{
	stages.clear();
	vkDestroyPipeline(appState.Device, pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;
	vkDestroyPipelineLayout(appState.Device, pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;
}

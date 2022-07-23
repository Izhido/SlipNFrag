#include "Pipeline.h"
#include "AppState.h"

void Pipeline::Delete(AppState& appState)
{
	vkDestroyPipeline(appState.Device, pipeline, nullptr);
	pipeline = VK_NULL_HANDLE;
	vkDestroyPipelineLayout(appState.Device, pipelineLayout, nullptr);
	pipelineLayout = VK_NULL_HANDLE;
}

#include "Pipeline.h"
#include "AppState.h"

void Pipeline::Delete(AppState& appState) const
{
	vkDestroyPipeline(appState.Device, pipeline, nullptr);
	vkDestroyPipelineLayout(appState.Device, pipelineLayout, nullptr);
}

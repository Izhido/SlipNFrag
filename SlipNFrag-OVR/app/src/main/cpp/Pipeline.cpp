#include "Pipeline.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"

void Pipeline::Delete(AppState& appState)
{
	VC(appState.Device.vkDestroyPipeline(appState.Device.device, pipeline, nullptr));
	VC(appState.Device.vkDestroyPipelineLayout(appState.Device.device, pipelineLayout, nullptr));
}

#include "PipelineDescriptorResources.h"
#include "AppState.h"

void PipelineDescriptorResources::Delete(AppState& appState)
{
	if (created)
	{
		vkDestroyDescriptorPool(appState.Device, descriptorPool, nullptr);
	}
}

#include "PipelineDescriptorResources.h"
#include "AppState.h"

void PipelineDescriptorResources::Delete(AppState& appState) const
{
	if (created)
	{
		vkDestroyDescriptorPool(appState.Device, descriptorPool, nullptr);
	}
}

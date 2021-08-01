#include "UpdatablePipelineDescriptorResources.h"
#include "AppState.h"

void UpdatablePipelineDescriptorResources::Delete(AppState& appState)
{
	if (created)
	{
		vkDestroyDescriptorPool(appState.Device, descriptorPool, nullptr);
		descriptorSetLayouts.clear();
		descriptorSets.clear();
		bound.clear();
		created = false;
	}
}

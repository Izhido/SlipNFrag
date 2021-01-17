#include "UpdatablePipelineDescriptorResources.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"

void UpdatablePipelineDescriptorResources::Delete(AppState& appState)
{
	if (created)
	{
		VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, descriptorPool, nullptr));
		descriptorSets.clear();
		bound.clear();
		created = false;
	}
}

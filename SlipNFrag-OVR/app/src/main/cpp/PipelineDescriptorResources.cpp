#include "PipelineDescriptorResources.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"

void PipelineDescriptorResources::Delete(AppState& appState)
{
	if (created)
	{
		VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, descriptorPool, nullptr));
	}
}

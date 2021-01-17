#include "CachedPipelineDescriptorResources.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"

void CachedPipelineDescriptorResources::Delete(AppState& appState)
{
	if (created)
	{
		VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, descriptorPool, nullptr));
		descriptorSets.clear();
		cache.clear();
		index = 0;
		created = false;
	}
}

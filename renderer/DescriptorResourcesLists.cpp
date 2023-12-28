#include "DescriptorResourcesLists.h"
#include "AppState.h"

void DescriptorResourcesLists::Delete(AppState& appState)
{
	if (!created)
	{
		return;
	}
	vkDestroyDescriptorPool(appState.Device, descriptorPool, nullptr);
	descriptorSetLayouts.clear();
	descriptorSets.clear();
	bound.clear();
	created = false;
}

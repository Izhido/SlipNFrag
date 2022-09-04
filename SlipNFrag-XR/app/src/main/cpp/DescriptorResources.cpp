#include "DescriptorResources.h"
#include "AppState.h"

void DescriptorResources::Delete(AppState& appState) const
{
	if (created)
	{
		vkDestroyDescriptorPool(appState.Device, descriptorPool, nullptr);
	}
}

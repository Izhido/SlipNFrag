#include "Image.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"

void Image::Delete(AppState& appState)
{
	if (view != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkDestroyImageView(appState.Device.device, view, nullptr));
	}
	if (image != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkDestroyImage(appState.Device.device, image, nullptr));
	}
	if (memory != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkFreeMemory(appState.Device.device, memory, nullptr));
	}
}

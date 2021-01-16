#include "MemoryAllocateInfo.h"
#include "AppState.h"
#include <android/log.h>
#include "sys_ovr.h"

void createMemoryAllocateInfo(AppState& appState, VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties, VkMemoryAllocateInfo& memoryAllocateInfo)
{
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	auto found = false;
	for (auto i = 0; i < appState.Context.device->physicalDeviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) != 0)
		{
			const VkFlags propertyFlags = appState.Context.device->physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags;
			if ((propertyFlags & properties) == properties)
			{
				found = true;
				memoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
	}
	if (!found)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "createMemoryAllocateInfo(): Memory type %d with properties %d not found.", memoryRequirements.memoryTypeBits, properties);
		vrapi_Shutdown();
		exit(0);
	}
}

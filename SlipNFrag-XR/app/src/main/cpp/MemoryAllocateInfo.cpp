#include "MemoryAllocateInfo.h"
#include "AppState.h"
#include <android/log.h>
#include "Utils.h"

void createMemoryAllocateInfo(AppState& appState, VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties, VkMemoryAllocateInfo& memoryAllocateInfo)
{
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;

	auto found = false;
	for (auto i = 0; i < appState.MemoryProperties.memoryTypeCount; i++)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) != 0)
		{
			const VkFlags propertyFlags = appState.MemoryProperties.memoryTypes[i].propertyFlags;
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
		THROW(Fmt("createMemoryAllocateInfo(): Memory type %d with properties %d not found.", memoryRequirements.memoryTypeBits, properties));
	}
}

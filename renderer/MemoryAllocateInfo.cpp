#include "MemoryAllocateInfo.h"
#include "AppState.h"
#include "Utils.h"

bool updateMemoryAllocateInfo(AppState& appState, VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags& properties, VkMemoryAllocateInfo& memoryAllocateInfo, bool throwOnNotFound)
{
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;

	auto found = false;
	VkMemoryPropertyFlags foundProperties = 0;
	for (auto i = 0; i < appState.MemoryProperties.memoryTypeCount; i++)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) != 0)
		{
			const VkFlags propertyFlags = appState.MemoryProperties.memoryTypes[i].propertyFlags;
			if ((propertyFlags & properties) == properties)
			{
				found = true;
				foundProperties = propertyFlags;
				memoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
	}

	if (!found && throwOnNotFound)
	{
		THROW(Fmt("createMemoryAllocateInfo(): Memory type %d with properties %d not found.", memoryRequirements.memoryTypeBits, properties));
	}
	else
	{
		properties = foundProperties;
	}

	return found;
}

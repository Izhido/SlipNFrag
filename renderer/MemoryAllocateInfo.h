#pragma once

#include <vulkan/vulkan.h>

struct AppState;

bool createMemoryAllocateInfo(AppState& appState, VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties, VkMemoryAllocateInfo& memoryAllocateInfo, bool throwOnNotFound);

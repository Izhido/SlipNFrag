#pragma once

#include <vulkan/vulkan.h>

struct AppState;

void createMemoryAllocateInfo(AppState& appState, VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties, VkMemoryAllocateInfo& memoryAllocateInfo);
void createMemoryAllocateInfo(AppState& appState, VkMemoryRequirements2& memoryRequirements, VkMemoryPropertyFlags properties, VkMemoryAllocateInfo& memoryAllocateInfo);

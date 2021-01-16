#pragma once

#include <vulkan/vulkan.h>

#define VK(func) checkErrors(func, #func);
#define VC(func) func;

void checkErrors(VkResult result, const char *function);
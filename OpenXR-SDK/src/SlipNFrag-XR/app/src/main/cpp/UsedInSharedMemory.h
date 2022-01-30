#pragma once

#include "SharedMemory.h"

struct UsedInSharedMemory
{
	SharedMemory* memory;
	VkDeviceSize used;
};

#pragma once

#include "Pipeline.h"
#include <vulkan/vulkan.h>

struct Scene;

template <typename Loaded, typename Sorted>
struct PipelineWithSorted : Pipeline
{
	int last;
	std::vector<Loaded> loaded;
	std::list<Sorted> sorted;
	VkDeviceSize vertexBase;
	VkDeviceSize indexBase;

	void Allocate(int last);
	void SetBases(VkDeviceSize vertexBase, VkDeviceSize indexBase);
	void ScaleIndexBase(int scale);
};

#define PIPELINEWITHSORTED_CPP
#include "PipelineWithSorted.cpp"
#undef PIPELINEWITHSORTED_CPP

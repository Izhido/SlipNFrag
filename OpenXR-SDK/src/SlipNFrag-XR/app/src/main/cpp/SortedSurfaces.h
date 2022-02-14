#pragma once

#include <unordered_map>
#include <list>
#include <vulkan/vulkan.h>
#include "SortedSurfaceLightmap.h"
#include "LoadedSurface.h"

struct SortedSurfaces
{
	std::unordered_map<VkDescriptorSet, std::list<SortedSurfaceLightmap>::iterator> added;
	std::list<SortedSurfaceLightmap> surfaces;
	std::list<SortedSurfaceLightmap> surfacesRotated;
	std::list<SortedSurfaceLightmap> fences;
	std::list<SortedSurfaceLightmap> fencesRotated;

	void Initialize(std::list<SortedSurfaceLightmap>& sorted);
	void Sort(LoadedSurface& loaded, int index, std::list<SortedSurfaceLightmap>& sorted);
};

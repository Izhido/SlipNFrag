#pragma once

#include "Pipeline.h"
#include <vulkan/vulkan.h>
#include "SortedSurfaceLightmapsWithTextures.h"

struct Scene;

template <typename Loaded, typename Sorted>
struct PipelineWithSorted : Pipeline
{
	int last;
	std::vector<Loaded> loaded;
	Sorted sorted;
	VkDeviceSize vertexBase;
	VkDeviceSize indexBase;

	void Allocate(int last)
	{
		this->last = last;
		if (last >= loaded.size())
		{
			loaded.resize(last + 1);
		}
	}

	void SetBases(VkDeviceSize vertexBase, VkDeviceSize indexBase)
	{
		this->vertexBase = vertexBase;
		this->indexBase = indexBase;
	}

	void ScaleIndexBase(int scale)
	{
		indexBase *= scale;
	}

	void Render(VkCommandBuffer commandBuffer)
	{
		if (std::is_same<Sorted, SortedSurfaceLightmapsWithTextures>::value)
		{
			VkDeviceSize index = 0;
			for (auto l = 0; l < sorted.count; l++)
			{
                const auto& lightmap = sorted.lightmaps[l];
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &lightmap.lightmap, 0, nullptr);
				for (auto t = 0; t < lightmap.count; t++)
				{
                    const auto& texture = lightmap.textures[t];
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1, &texture.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, texture.indexCount, 1, index, 0, 0);
					index += texture.indexCount;
				}
			}
		}
	}
};

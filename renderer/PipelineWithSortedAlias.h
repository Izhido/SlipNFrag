#pragma once

#include "Pipeline.h"
#include <type_traits>
#include "SortedAliasColormaps.h"
#include "SortedAliasIndices.h"
#include "AliasColoredLightsPushConstants.h"

template <typename Loaded, typename Sorted>
struct PipelineWithSortedAlias : Pipeline
{
	int last;
    int attributeBaseIndex;
	std::vector<Loaded> loaded;
	Sorted sorted;

	void Allocate(int last)
	{
		this->last = last;
		if (last >= loaded.size())
		{
			loaded.resize(last + 1);
		}
	}

	void Render(VkCommandBuffer commandBuffer, VkDescriptorSet hostColormap)
	{
		static_assert(std::is_same<Sorted, SortedAliasColormaps>::value, "PipelineWithSortedAlias::Render(VkCommandBuffer, VkDescriptorSet) is available only for Sorted=SortedAliasColormaps");
        std::array<VkBuffer, 2> vertexBuffers { };
        std::array<VkDeviceSize, 2> offsets { 0, 0 };
		for (auto c = 0; c < sorted.count; c++)
		{
			auto& colormap = sorted.colormaps[c];
			if (colormap.colormap == VK_NULL_HANDLE)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &hostColormap, 0, nullptr);
			}
			else
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &colormap.colormap, 0, nullptr);
			}
			for (auto ix = 0; ix < colormap.count; ix++)
			{
				auto& indices = colormap.indices[ix];
				vkCmdBindIndexBuffer(commandBuffer, indices.indices, 0, indices.indexType);
				for (auto t = 0; t < indices.count; t++)
				{
					auto& texture = indices.textures[t];
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &texture.texture, 0, nullptr);
					for (auto v = 0; v < texture.count; v++)
					{
						auto& vertices = texture.vertices[v];
						vertexBuffers[0] = vertices.vertices;
                        vertexBuffers[1] = vertices.texCoords;
						vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers.data(), offsets.data());
						vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(int), &attributeBaseIndex);
						for (auto i : vertices.entries)
						{
							auto& l = loaded[i];
							vkCmdDrawIndexed(commandBuffer, l.count, 1, l.indices.indices.firstIndex, 0, i);
						}
					}
				}
			}
		}
	}

	void Render(VkCommandBuffer commandBuffer)
	{
		static_assert(std::is_same<Sorted, SortedAliasIndices>::value, "PipelineWithSortedAlias::Render(VkCommandBuffer) is available only for Sorted=SortedAliasIndices");
        std::array<VkBuffer, 2> vertexBuffers { };
        std::array<VkDeviceSize, 2> offsets { 0, 0 };
		for (auto ix = 0; ix < sorted.count; ix++)
		{
			auto& indices = sorted.indices[ix];
			vkCmdBindIndexBuffer(commandBuffer, indices.indices, 0, indices.indexType);
			for (auto t = 0; t < indices.count; t++)
			{
				auto& texture = indices.textures[t];
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &texture.texture, 0, nullptr);
				for (auto v = 0; v < texture.count; v++)
				{
					auto& vertices = texture.vertices[v];
                    vertexBuffers[0] = vertices.vertices;
                    vertexBuffers[1] = vertices.texCoords;
					vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers.data(), offsets.data());
					vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 20, sizeof(int), &attributeBaseIndex);
					for (auto i : vertices.entries)
					{
						auto& l = loaded[i];
						vkCmdDrawIndexed(commandBuffer, l.count, 1, l.indices.indices.firstIndex, 0, i);
					}
				}
			}
		}
	}
};

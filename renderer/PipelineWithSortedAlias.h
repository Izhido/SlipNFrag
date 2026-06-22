#pragma once

#include "Pipeline.h"
#include <type_traits>
#include "SortedAliasColormaps.h"
#include "SortedAliasIndices.h"
#include "AliasPushConstants.h"
#include "AliasColoredLightsPushConstants.h"

template <typename Loaded, typename Sorted>
struct PipelineWithSortedAlias : Pipeline
{
	int last;
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

	void SetTransform(const Loaded& loaded, PushConstants& pushConstants)
	{
		pushConstants.pushConstants[0] = loaded.transform[0][0];
		pushConstants.pushConstants[1] = loaded.transform[1][0];
		pushConstants.pushConstants[2] = loaded.transform[2][0];
		pushConstants.pushConstants[4] = loaded.transform[0][1];
		pushConstants.pushConstants[5] = loaded.transform[1][1];
		pushConstants.pushConstants[6] = loaded.transform[2][1];
		pushConstants.pushConstants[8] = loaded.transform[0][2];
		pushConstants.pushConstants[9] = loaded.transform[1][2];
		pushConstants.pushConstants[10] = loaded.transform[2][2];
		pushConstants.pushConstants[12] = loaded.transform[0][3];
		pushConstants.pushConstants[13] = loaded.transform[1][3];
		pushConstants.pushConstants[14] = loaded.transform[2][3];
	}

	template <typename PushConstantsType>
	void Render(VkCommandBuffer commandBuffer, PushConstantsType& pushConstants, VkDescriptorSet hostColormap, VkDeviceSize lightBase)
	{
		static_assert(std::is_same<Sorted, SortedAliasColormaps>::value, "PipelineWithSortedAlias::Render(VkCommandBuffer, PushConstantsType&, VkDescriptorSet, int) is available only for Sorted=SortedAliasColormaps");
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
						for (auto i : vertices.entries)
						{
							auto& l = loaded[i];
							SetTransform(l, pushConstants);
							pushConstants.lightIndex = (int)lightBase + l.firstLight;
							vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantsType), &pushConstants);
							vkCmdDrawIndexed(commandBuffer, l.count, 1, l.indices.indices.firstIndex, 0, 0);
						}
					}
				}
			}
		}
	}

	template <typename PushConstantsType>
	void Render(VkCommandBuffer commandBuffer, PushConstantsType& pushConstants, VkDeviceSize lightBase)
	{
		static_assert(std::is_same<Sorted, SortedAliasIndices>::value, "PipelineWithSortedAlias::Render(VkCommandBuffer, PushConstantsType&, int) is available only for Sorted=SortedAliasIndices");
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
					for (auto i : vertices.entries)
					{
						auto& l = loaded[i];
						SetTransform(l, pushConstants);
						pushConstants.lightIndex = (int)lightBase + l.firstLight;
						vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsType), &pushConstants);
						vkCmdDrawIndexed(commandBuffer, l.count, 1, l.indices.indices.firstIndex, 0, 0);
					}
				}
			}
		}
	}
};

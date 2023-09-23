//
//  Surfaces.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 25/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "Surfaces.h"
#include "vid_visionos.h"
#include "d_lists.h"
#import "Lightmap.h"

void Surfaces::Sort(std::unordered_map<void*, SortedSurfaceLightmap>& sorted, NSUInteger& verticesSize, NSUInteger& indicesSize)
{
	for (auto s = 0; s <= d_lists.last_surface; s++)
	{
		auto& surface = d_lists.surfaces[s];
		auto face = (msurface_t*)surface.face;

		auto& lightmap = sorted[surface.face];
		
		if (!lightmap.texturePositionSet)
		{
			auto texinfo = face->texinfo;

			lightmap.texturePosition[0] = texinfo->vecs[0][0];
			lightmap.texturePosition[1] = texinfo->vecs[0][1];
			lightmap.texturePosition[2] = texinfo->vecs[0][2];
			lightmap.texturePosition[3] = texinfo->vecs[0][3];
			lightmap.texturePosition[4] = texinfo->vecs[1][0];
			lightmap.texturePosition[5] = texinfo->vecs[1][1];
			lightmap.texturePosition[6] = texinfo->vecs[1][2];
			lightmap.texturePosition[7] = texinfo->vecs[1][3];
			
			lightmap.texturePosition[8] = face->texturemins[0];
			lightmap.texturePosition[9] = face->texturemins[1];

			lightmap.texturePosition[10] = face->extents[0];
			lightmap.texturePosition[11] = face->extents[1];

			lightmap.texturePosition[12] = texinfo->texture->width;
			lightmap.texturePosition[13] = texinfo->texture->height;
			
			lightmap.texturePositionSet = true;
		}

		auto& texture = lightmap.textures[surface.data];
		
		texture.entries.push_back(s);
		
		auto numedges = face->numedges;
		
		auto indices = (numedges - 2) * 3;

		texture.indices += indices;
		
		verticesSize += numedges * 3 * sizeof(float);
		indicesSize += indices * sizeof(uint32_t);
	}
}

void Surfaces::Fill(std::unordered_map<void*, SortedSurfaceLightmap>& sorted, float*& vertices, uint32_t*& indices, uint32_t& base, std::unordered_map<void*, NSUInteger>* lightmapIndex, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache)
{
	for (auto& lightmap : sorted)
	{
		for (auto& texture : lightmap.second.textures)
		{
			for (auto s : texture.second.entries)
			{
				auto& surface = d_lists.surfaces[s];
				auto face = (msurface_t*)surface.face;
				auto model = (model_t*)surface.model;
				auto edge = model->surfedges[face->firstedge];
				unsigned int index;
				if (edge >= 0)
				{
					index = model->edges[edge].v[0];
				}
				else
				{
					index = model->edges[-edge].v[1];
				}
				auto vertexes = (float*)model->vertexes;
				auto source = vertexes + index * 3;
				*vertices++ = *source++;
				*vertices++ = *source++;
				*vertices++ = *source;
				auto next_front = 0;
				auto next_back = face->numedges;
				auto use_back = false;
				for (auto j = 1; j < face->numedges; j++)
				{
					if (use_back)
					{
						next_back--;
						edge = model->surfedges[face->firstedge + next_back];
					}
					else
					{
						next_front++;
						edge = model->surfedges[face->firstedge + next_front];
					}
					use_back = !use_back;
					if (edge >= 0)
					{
						index = model->edges[edge].v[0];
					}
					else
					{
						index = model->edges[-edge].v[1];
					}
					source = vertexes + index * 3;
					*vertices++ = *source++;
					*vertices++ = *source++;
					*vertices++ = *source;
				}
				
				*indices++ = base++;
				*indices++ = base++;
				*indices++ = base++;
				auto revert = true;
				for (auto j = 1; j < face->numedges - 2; j++)
				{
					if (revert)
					{
						*indices++ = base;
						*indices++ = base - 1;
						*indices++ = base - 2;
					}
					else
					{
						*indices++ = base - 2;
						*indices++ = base - 1;
						*indices++ = base;
					}
					base++;
					revert = !revert;
				}
				
				if (!lightmap.second.lightmapSet)
				{
					auto entry = lightmapIndex->find(surface.face);
					if (entry == lightmapIndex->end())
					{
						auto newLightmap = [Lightmap new];
						newLightmap.descriptor = [MTLTextureDescriptor new];
						newLightmap.descriptor.pixelFormat = MTLPixelFormatR16Unorm;
						newLightmap.descriptor.width = surface.lightmap_width;
						newLightmap.descriptor.height = surface.lightmap_height;
						newLightmap.descriptor.resourceOptions = MTLResourceCPUCacheModeWriteCombined;
						newLightmap.descriptor.usage = MTLTextureUsageShaderRead;
						newLightmap.texture = [device newTextureWithDescriptor:newLightmap.descriptor];
						newLightmap.region = MTLRegionMake2D(0, 0, surface.lightmap_width, surface.lightmap_height);
						
						newLightmap.createdCount = surface.created;

						[newLightmap.texture replaceRegion:newLightmap.region mipmapLevel:0 withBytes:d_lists.lightmap_texels.data() + surface.lightmap_texels bytesPerRow:newLightmap.region.size.width * sizeof(uint16_t)];

						lightmapIndex->insert({surface.face, perDrawable.lightmapCache.count});
						
						lightmap.second.lightmap = (uint32_t)perDrawable.lightmapCache.count;
						
						[perDrawable.lightmapCache addObject:newLightmap];
					}
					else
					{
						lightmap.second.lightmap = (uint32_t)entry->second;

						auto existingLightmap = perDrawable.lightmapCache[lightmap.second.lightmap];
						if (existingLightmap.createdCount != surface.created)
						{
							[existingLightmap.texture replaceRegion:existingLightmap.region mipmapLevel:0 withBytes:d_lists.lightmap_texels.data() + surface.lightmap_texels bytesPerRow:existingLightmap.region.size.width * sizeof(uint16_t)];
						}
					}

					lightmap.second.lightmapSet = true;
				}

				if (!texture.second.set)
				{
					auto entry = textureIndex.find(surface.data);
					if (entry == textureIndex.end())
					{
						auto newTexture = [Texture new];
						newTexture.descriptor = [MTLTextureDescriptor new];
						newTexture.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
						newTexture.descriptor.width = surface.width;
						newTexture.descriptor.height = surface.height;
						newTexture.descriptor.mipmapLevelCount = surface.mips;
						newTexture.descriptor.usage = MTLTextureUsageShaderRead;
						newTexture.texture = [device newTextureWithDescriptor:newTexture.descriptor];
						newTexture.region = MTLRegionMake2D(0, 0, surface.width, surface.height);
						
						auto data = (unsigned char*)surface.data;
						auto region = newTexture.region;
						for (auto m = 0; m < surface.mips; m++)
						{
							[newTexture.texture replaceRegion:region mipmapLevel:m withBytes:data bytesPerRow:region.size.width];
							data += region.size.width * region.size.height;
							region.size.width /= 2;
							region.size.height /= 2;
						}

						textureIndex.insert({surface.data, textureCache.count});
						
						texture.second.texture = (uint32_t)textureCache.count;
						
						[textureCache addObject:newTexture];
					}
					else
					{
						texture.second.texture = (uint32_t)entry->second;
					}

					texture.second.set = true;
				}
			}
		}
	}
}

void Surfaces::Render(std::unordered_map<void*, SortedSurfaceLightmap>& sorted, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger indexBase, id<MTLSamplerState> planarSamplerState, id<MTLSamplerState> lightmapSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState)
{
	if (sorted.size() >= 0)
	{
		[commandEncoder setRenderPipelineState:pipeline];
		[commandEncoder setDepthStencilState:depthStencilState];
		[commandEncoder setVertexBytes:&vertexTransformMatrix length:sizeof(vertexTransformMatrix) atIndex:1];
		[commandEncoder setVertexBytes:&viewMatrix length:sizeof(viewMatrix) atIndex:2];
		[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:3];
		[commandEncoder setFragmentTexture:perDrawable.palette.texture atIndex:0];
		[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:0];
		[commandEncoder setFragmentTexture:perDrawable.colormap.texture atIndex:1];
		[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:1];
		[commandEncoder setVertexBuffer:perDrawable.surfaceVertices offset:0 atIndex:0];

		for (auto& lightmap : sorted)
		{
			[commandEncoder setVertexBytes:&lightmap.second.texturePosition length:sizeof(lightmap.second.texturePosition) atIndex:4];
			[commandEncoder setFragmentTexture:perDrawable.lightmapCache[lightmap.second.lightmap].texture atIndex:2];
			[commandEncoder setFragmentSamplerState:lightmapSamplerState atIndex:2];

			for (auto& texture : lightmap.second.textures)
			{
				[commandEncoder setFragmentTexture:textureCache[texture.second.texture].texture atIndex:3];
				[commandEncoder setFragmentSamplerState:textureSamplerState atIndex:3];

				[commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:(NSUInteger)texture.second.indices indexType:MTLIndexTypeUInt32 indexBuffer:perDrawable.indices indexBufferOffset:indexBase];
				
				indexBase += (NSUInteger)texture.second.indices * sizeof(uint32_t);
			}
		}
	}
}

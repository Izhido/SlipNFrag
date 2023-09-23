//
//  Turbulent.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 2/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "Turbulent.h"
#include "vid_visionos.h"
#include "d_lists.h"

void Turbulent::Sort(std::unordered_map<void*, SortedTurbulentTexture>& sorted, NSUInteger& verticesSize, NSUInteger& indicesSize)
{
	for (auto t = 0; t <= d_lists.last_turbulent; t++)
	{
		auto& turbulent = d_lists.turbulent[t];
		auto face = (msurface_t*)turbulent.face;

		auto& texture = sorted[turbulent.data];
		
		if (!texture.texturePositionSet)
		{
			auto texinfo = face->texinfo;

			texture.texturePosition[0] = texinfo->vecs[0][0];
			texture.texturePosition[1] = texinfo->vecs[0][1];
			texture.texturePosition[2] = texinfo->vecs[0][2];
			texture.texturePosition[3] = texinfo->vecs[0][3];
			texture.texturePosition[4] = texinfo->vecs[1][0];
			texture.texturePosition[5] = texinfo->vecs[1][1];
			texture.texturePosition[6] = texinfo->vecs[1][2];
			texture.texturePosition[7] = texinfo->vecs[1][3];
			
			texture.textureSize[0] = texinfo->texture->width;
			texture.textureSize[1] = texinfo->texture->height;
			
			texture.texturePositionSet = true;
		}

		texture.entries.push_back(t);
		
		auto numedges = face->numedges;
		
		auto indices = (numedges - 2) * 3;

		texture.indices += indices;
		
		verticesSize += numedges * 3 * sizeof(float);
		indicesSize += indices * sizeof(uint32_t);
	}
}

void Turbulent::Fill(std::unordered_map<void*, SortedTurbulentTexture>& sorted, float*& vertices, uint32_t*& indices, uint32_t& base, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache)
{
	for (auto& texture : sorted)
	{
		for (auto t : texture.second.entries)
		{
			auto& turbulent = d_lists.turbulent[t];
			auto face = (msurface_t*)turbulent.face;
			auto model = (model_t*)turbulent.model;
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
			
			if (!texture.second.textureSet)
			{
				auto entry = textureIndex.find(turbulent.data);
				if (entry == textureIndex.end())
				{
					auto newTexture = [Texture new];
					newTexture.descriptor = [MTLTextureDescriptor new];
					newTexture.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
					newTexture.descriptor.width = turbulent.width;
					newTexture.descriptor.height = turbulent.height;
					newTexture.descriptor.mipmapLevelCount = turbulent.mips;
					newTexture.descriptor.usage = MTLTextureUsageShaderRead;
					newTexture.texture = [device newTextureWithDescriptor:newTexture.descriptor];
					newTexture.region = MTLRegionMake2D(0, 0, turbulent.width, turbulent.height);
					
					auto data = (unsigned char*)turbulent.data;
					auto region = newTexture.region;
					for (auto m = 0; m < turbulent.mips; m++)
					{
						[newTexture.texture replaceRegion:region mipmapLevel:m withBytes:data bytesPerRow:region.size.width];
						data += region.size.width * region.size.height;
						region.size.width /= 2;
						region.size.height /= 2;
					}

					textureIndex.insert({turbulent.data, textureCache.count});
					
					texture.second.texture = (uint32_t)textureCache.count;
					
					[textureCache addObject:newTexture];
				}
				else
				{
					texture.second.texture = (uint32_t)entry->second;
				}

				texture.second.textureSet = true;
			}
		}
	}
}

void Turbulent::Render(std::unordered_map<void*, SortedTurbulentTexture>& sorted, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger indexBase, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState)
{
	if (sorted.size() >= 0)
	{
		auto time = (float)cl.time;

		[commandEncoder setRenderPipelineState:pipeline];
		[commandEncoder setDepthStencilState:depthStencilState];
		[commandEncoder setVertexBytes:&vertexTransformMatrix length:sizeof(vertexTransformMatrix) atIndex:1];
		[commandEncoder setVertexBytes:&viewMatrix length:sizeof(viewMatrix) atIndex:2];
		[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:3];
		[commandEncoder setFragmentTexture:perDrawable.palette.texture atIndex:0];
		[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:0];
		[commandEncoder setVertexBuffer:perDrawable.surfaceVertices offset:0 atIndex:0];
		[commandEncoder setFragmentBytes:&time length:sizeof(time) atIndex:0];

		for (auto& texture : sorted)
		{
			[commandEncoder setVertexBytes:&texture.second.texturePosition length:sizeof(texture.second.texturePosition) atIndex:4];
			[commandEncoder setVertexBytes:&texture.second.textureSize length:sizeof(texture.second.textureSize) atIndex:5];
			[commandEncoder setFragmentTexture:textureCache[texture.second.texture].texture atIndex:1];
			[commandEncoder setFragmentSamplerState:textureSamplerState atIndex:1];

			[commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:(NSUInteger)texture.second.indices indexType:MTLIndexTypeUInt32 indexBuffer:perDrawable.indices indexBufferOffset:indexBase];
				
			indexBase += (NSUInteger)texture.second.indices * sizeof(uint32_t);
		}
	}
}

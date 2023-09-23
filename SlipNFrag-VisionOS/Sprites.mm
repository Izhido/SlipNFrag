//
//  Sprites.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 18/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "Sprites.h"
#include "vid_visionos.h"
#include "d_lists.h"

void Sprites::Sort(std::unordered_map<void*, SortedSpriteTexture>& sorted, NSUInteger& verticesSize, NSUInteger& indicesSize)
{
	for (auto t = 0; t <= d_lists.last_sprite; t++)
	{
		auto& sprite = d_lists.sprites[t];

		auto& texture = sorted[sprite.data];

		texture.entries.push_back(t);

		auto indices = 6;

		texture.indices += indices;

		verticesSize += sprite.count * 6 * sizeof(float);
		indicesSize += indices * sizeof(uint32_t);
	}
}

void Sprites::Fill(std::unordered_map<void*, SortedSpriteTexture>& sorted, float*& vertices, uint32_t*& indices, uint32_t& base, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache)
{
	for (auto& texture : sorted)
	{
		for (auto t : texture.second.entries)
		{
			auto& sprite = d_lists.sprites[t];
			
			if (!texture.second.set)
			{
				auto entry = textureIndex.find(sprite.data);
				if (entry == textureIndex.end())
				{
					auto newTexture = [Texture new];
					newTexture.descriptor = [MTLTextureDescriptor new];
					newTexture.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
					newTexture.descriptor.width = sprite.width;
					newTexture.descriptor.height = sprite.height;
					newTexture.descriptor.usage = MTLTextureUsageShaderRead;
					newTexture.texture = [device newTextureWithDescriptor:newTexture.descriptor];
					newTexture.region = MTLRegionMake2D(0, 0, sprite.width, sprite.height);
					
					[newTexture.texture replaceRegion:newTexture.region mipmapLevel:0 withBytes:sprite.data bytesPerRow:newTexture.region.size.width];

					textureIndex.insert({sprite.data, textureCache.count});
					
					texture.second.texture = (uint32_t)textureCache.count;
					
					[textureCache addObject:newTexture];
				}
				else
				{
					texture.second.texture = (uint32_t)entry->second;
				}

				texture.second.set = true;
			}


			auto vertexSource = d_lists.textured_vertices.data() + sprite.first_vertex * 3;
			auto attributeSource = d_lists.textured_attributes.data() + sprite.first_vertex * 2;
			for (auto i = 0; i < sprite.count; i++)
			{
				*vertices++ = *vertexSource++;
				*vertices++ = *vertexSource++;
				*vertices++ = *vertexSource++;
				*vertices++ = 1;
				*vertices++ = *attributeSource++;
				*vertices++ = *attributeSource++;
			}

			*indices++ = base;
			*indices++ = base + 1;
			*indices++ = base + 2;
			*indices++ = base + 1;
			*indices++ = base + 3;
			*indices++ = base + 2;
			
			base += 4;
		}
	}
}

void Sprites::Render(std::unordered_map<void*, SortedSpriteTexture>& sorted, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger indexBase, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState)
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
		[commandEncoder setVertexBuffer:perDrawable.texturedVertices offset:0 atIndex:0];

		for (auto& texture : sorted)
		{
			[commandEncoder setFragmentTexture:textureCache[texture.second.texture].texture atIndex:1];
			[commandEncoder setFragmentSamplerState:textureSamplerState atIndex:1];

			[commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:(NSUInteger)texture.second.indices indexType:MTLIndexTypeUInt32 indexBuffer:perDrawable.indices indexBufferOffset:indexBase];
				
			indexBase += (NSUInteger)texture.second.indices * sizeof(uint32_t);
		}
	}
}

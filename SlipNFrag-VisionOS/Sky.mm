//
//  Sky.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 15/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Sky.h"
#include "vid_visionos.h"
#include "d_lists.h"

void Sky::Sort(std::vector<SkyEntry>& skyEntries, NSUInteger& verticesSize, NSUInteger& indicesSize)
{
	for (auto s = 0; s <= d_lists.last_sky; s++)
	{
		auto& sky = d_lists.sky[s];
		
		skyEntries.emplace_back();
		auto& entry = skyEntries.back();
		
		entry.entry = s;

		entry.indices = 6;

		verticesSize = sky.count * 6 * sizeof(float);
		indicesSize += entry.indices * sizeof(uint32_t);
	}
}

void Sky::Fill(std::vector<SkyEntry>& skyEntries, float*& vertices, uint32_t*& indices, uint32_t& base, id<MTLDevice> device, PerDrawable* perDrawable, std::vector<unsigned char>& skyBuffer, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache)
{
	for (auto& entry : skyEntries)
	{
		auto& sky = d_lists.sky[entry.entry];

		auto width = sky.width;
		auto height = sky.height;
		auto doubleWidth = width * 2;

		size_t bufferSize = width * height;
		if (skyBuffer.size() != bufferSize)
		{
			skyBuffer.resize(bufferSize);
		}

		auto source = sky.data;
		auto target = skyBuffer.data();
		for (auto i = 0; i < height; i++)
		{
			memcpy(target, source, width);
			source += doubleWidth;
			target += width;
		}
		
		auto textureEntry = textureIndex.find(sky.data);
		if (textureEntry == textureIndex.end())
		{
			auto newTexture = [Texture new];
			newTexture.descriptor = [MTLTextureDescriptor new];
			newTexture.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
			newTexture.descriptor.width = width;
			newTexture.descriptor.height = height;
			newTexture.texture = [device newTextureWithDescriptor:newTexture.descriptor];
			newTexture.region = MTLRegionMake2D(0, 0, width, height);
			
			[newTexture.texture replaceRegion:newTexture.region mipmapLevel:0 withBytes:skyBuffer.data() bytesPerRow:newTexture.region.size.width];

			textureIndex.insert({sky.data, textureCache.count});
			
			entry.texture = (uint32_t)textureCache.count;
			
			[textureCache addObject:newTexture];
		}
		else
		{
			entry.texture = (uint32_t)textureEntry->second;
			
			auto texture = textureCache[entry.texture];
			
			[texture.texture replaceRegion:texture.region mipmapLevel:0 withBytes:skyBuffer.data() bytesPerRow:texture.region.size.width];
		}

		auto vertexSource = d_lists.textured_vertices.data() + sky.first_vertex * 3;
		auto attributeSource = d_lists.textured_attributes.data() + sky.first_vertex * 2;
		for (auto i = 0; i < sky.count; i++)
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

void Sky::Render(std::vector<SkyEntry>& skyEntries, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, simd_float4x4& headPose, cp_drawable_t& drawable, NSUInteger view, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState)
{
	if (skyEntries.size() >= 0)
	{
		auto colorTexture = cp_drawable_get_color_texture(drawable, view);
		
		float rotation[] =
		{
			-headPose.columns[2][0],
			headPose.columns[2][2],
			-headPose.columns[2][1],
			headPose.columns[0][0],
			-headPose.columns[0][2],
			headPose.columns[0][1],
			headPose.columns[1][0],
			-headPose.columns[1][2],
			headPose.columns[1][1],
			(float)colorTexture.width,
			(float)colorTexture.height,
			(float)std::max(colorTexture.width, colorTexture.height),
			skytime*skyspeed
		};

		[commandEncoder setRenderPipelineState:pipeline];
		[commandEncoder setDepthStencilState:depthStencilState];
		[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:1];
		[commandEncoder setFragmentTexture:perDrawable.palette.texture atIndex:0];
		[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:0];
		[commandEncoder setVertexBuffer:perDrawable.texturedVertices offset:0 atIndex:0];
		[commandEncoder setFragmentBytes:rotation length:sizeof(rotation) atIndex:0];

		auto indexBase = 0;
		
		for (auto& entry : skyEntries)
		{
			[commandEncoder setFragmentTexture:textureCache[entry.texture].texture atIndex:1];
			[commandEncoder setFragmentSamplerState:textureSamplerState atIndex:1];

			[commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:(NSUInteger)entry.indices indexType:MTLIndexTypeUInt32 indexBuffer:perDrawable.indices indexBufferOffset:indexBase];
				
			indexBase += (NSUInteger)entry.indices * sizeof(uint32_t);
		}
	}
}

//
//  Skybox.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 1/10/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Skybox.h"
#include "vid_visionos.h"
#include "d_lists.h"

void Skybox::Sort(std::vector<SkyEntry>& skyboxEntries, NSUInteger& verticesSize, NSUInteger& indicesSize)
{
	for (auto s = 0; s <= d_lists.last_skybox; s++)
	{
		skyboxEntries.emplace_back();
		auto& entry = skyboxEntries.back();
		
		entry.entry = s;

		entry.indices = 36;

		verticesSize += 24 * 4 * sizeof(float);
		indicesSize += entry.indices * sizeof(uint32_t);
	}
}

void Skybox::Fill(std::vector<SkyEntry>& skyboxEntries, float*& vertices, uint32_t*& indices, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache)
{
	for (auto& entry : skyboxEntries)
	{
		auto& skybox = d_lists.skyboxes[entry.entry];

		auto textureEntry = textureIndex.find(skybox.textures);
		if (textureEntry == textureIndex.end())
		{
			auto size = skybox.textures->texture->width;
			auto newTexture = [Texture new];
			newTexture.descriptor = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatR8Unorm size:size mipmapped:NO];
			newTexture.descriptor.usage = MTLTextureUsageShaderRead;
			newTexture.texture = [device newTextureWithDescriptor:newTexture.descriptor];
			newTexture.region = MTLRegionMake2D(0, 0, size, size);

			for (auto s = 0; s < 6; s++)
			{
				std::string name;

				switch (s)
				{
					case 0:
						name = "ft";
						break;
					case 1:
						name = "bk";
						break;
					case 2:
						name = "up";
						break;
					case 3:
						name = "dn";
						break;
					case 4:
						name = "rt";
						break;
					default:
						name = "lf";
						break;
				}

				NSUInteger slice = 0;
				while (slice < 5)
				{
					if (name == std::string(skybox.textures[slice].texture->name))
					{
						break;
					}
					slice++;
				}
				
				[newTexture.texture replaceRegion:newTexture.region mipmapLevel:0 slice:s withBytes:(((byte*)skybox.textures[slice].texture) + sizeof(texture_t)) bytesPerRow:newTexture.region.size.width bytesPerImage:newTexture.region.size.width * newTexture.region.size.height];
			}

			textureIndex.insert({skybox.textures, textureCache.count});
			
			entry.texture = (uint32_t)textureCache.count;
			
			[textureCache addObject:newTexture];
		}
		else
		{
			entry.texture = (uint32_t)textureEntry->second;
		}

		constexpr float vertexSource[] =
		{
			-0.5,  0.5,  0.5, 1.0,
			 0.5,  0.5,  0.5, 1.0,
			 0.5,  0.5, -0.5, 1.0,
			-0.5,  0.5, -0.5, 1.0,
			-0.5, -0.5, -0.5, 1.0,
			 0.5, -0.5, -0.5, 1.0,
			 0.5, -0.5,  0.5, 1.0,
			-0.5, -0.5,  0.5, 1.0,
			-0.5, -0.5,  0.5, 1.0,
			 0.5, -0.5,  0.5, 1.0,
			 0.5,  0.5,  0.5, 1.0,
			-0.5,  0.5,  0.5, 1.0,
			 0.5, -0.5, -0.5, 1.0,
			-0.5, -0.5, -0.5, 1.0,
			-0.5,  0.5, -0.5, 1.0,
			 0.5,  0.5, -0.5, 1.0,
			-0.5, -0.5, -0.5, 1.0,
			-0.5, -0.5,  0.5, 1.0,
			-0.5,  0.5,  0.5, 1.0,
			-0.5,  0.5, -0.5, 1.0,
			 0.5, -0.5,  0.5, 1.0,
			 0.5, -0.5, -0.5, 1.0,
			 0.5,  0.5, -0.5, 1.0,
			 0.5,  0.5,  0.5, 1.0
		};

		memcpy(vertices, vertexSource, 24 * 4 * sizeof(float));
		vertices += 24 * 4;
		
		constexpr uint32_t indexSource[] =
		{
			 0,  3,  2,  2,  1,  0,
			 4,  7,  6,  6,  5,  4,
			 8, 11, 10, 10,  9,  8,
			12, 15, 14, 14, 13, 12,
			16, 19, 18, 18, 17, 16,
			20, 23, 22, 22, 21, 20
		};
		
		memcpy(indices, indexSource, entry.indices * sizeof(uint32_t));
		indices += entry.indices;
	}
}

void Skybox::Render(std::vector<SkyEntry>& skyboxEntries, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, simd_float4x4& headPose, cp_drawable_t& drawable, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState)
{
	if (skyboxEntries.size() >= 0)
	{
		auto cameraMatrix = headPose;
		cameraMatrix.columns[3][0] = 0;
		cameraMatrix.columns[3][1] = 0;
		cameraMatrix.columns[3][2] = 0;
		cameraMatrix.columns[3][3] = 1;
		
		auto axis = simd_make_float3(0, 1, 0);
		auto rotation = simd_quaternion(M_PI / 2, axis);

		auto rotatedMatrix = simd_mul(simd_matrix4x4(rotation), cameraMatrix);
		
		auto viewMatrix = simd_transpose(rotatedMatrix);

		[commandEncoder setRenderPipelineState:pipeline];
		[commandEncoder setDepthStencilState:depthStencilState];
		[commandEncoder setVertexBytes:&viewMatrix length:sizeof(viewMatrix) atIndex:1];
		[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:2];
		[commandEncoder setFragmentTexture:perDrawable.palette.texture atIndex:0];
		[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:0];
		[commandEncoder setVertexBuffer:perDrawable.skyboxVertices offset:0 atIndex:0];

		auto indexBase = 0;
		
		for (auto& entry : skyboxEntries)
		{
			[commandEncoder setFragmentTexture:textureCache[entry.texture].texture atIndex:1];
			[commandEncoder setFragmentSamplerState:textureSamplerState atIndex:1];

			[commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:(NSUInteger)entry.indices indexType:MTLIndexTypeUInt32 indexBuffer:perDrawable.indices indexBufferOffset:indexBase];
				
			indexBase += (NSUInteger)entry.indices * sizeof(uint32_t);
		}
	}
}

//
//  Viewmodel.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 14/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "Viewmodel.h"
#include "vid_visionos.h"
#include "d_lists.h"

void Viewmodel::Sort(std::unordered_map<void*, SortedAliasTexture>& sorted, NSUInteger& verticesSize)
{
	for (auto a = 0; a <= d_lists.last_viewmodel; a++)
	{
		auto& viewmodel = d_lists.viewmodels[a];
		auto aliashdr = (aliashdr_t *)viewmodel.aliashdr;
		auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);

		auto& texture = sorted[viewmodel.data];
		
		texture.entries.emplace_back();
		
		auto& entry = texture.entries.back();

		entry.entry = a;

		entry.transform[0] = viewmodel.transform[0][0];
		entry.transform[1] = viewmodel.transform[1][0];
		entry.transform[2] = viewmodel.transform[2][0];
		entry.transform[3] = 0;
		entry.transform[4] = viewmodel.transform[0][1];
		entry.transform[5] = viewmodel.transform[1][1];
		entry.transform[6] = viewmodel.transform[2][1];
		entry.transform[7] = 0;
		entry.transform[8] = viewmodel.transform[0][2];
		entry.transform[9] = viewmodel.transform[1][2];
		entry.transform[10] = viewmodel.transform[2][2];
		entry.transform[11] = 0;
		entry.transform[12] = viewmodel.transform[0][3];
		entry.transform[13] = viewmodel.transform[1][3];
		entry.transform[14] = viewmodel.transform[2][3];
		entry.transform[15] = 1;

		entry.indices = 3 * mdl->numtris;
		
		entry.vertices = 2 * 7 * viewmodel.vertex_count;
		verticesSize += entry.vertices * sizeof(float);
	}
}

void Viewmodel::Fill(std::unordered_map<void*, SortedAliasTexture>& sorted, float*& vertices, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache, std::unordered_map<void*, NSUInteger>& aliasIndicesIndex, NSMutableArray<id<MTLBuffer>>* aliasIndicesCache, std::unordered_map<void*, NSUInteger>& colormapIndex, NSMutableArray<Texture*>* colormapCache)
{
	for (auto& texture : sorted)
	{
		for (auto& entry : texture.second.entries)
		{
			auto& viewmodel = d_lists.viewmodels[entry.entry];

			auto width = viewmodel.width;
			auto height = viewmodel.height;

			if (!texture.second.set)
			{
				auto textureEntry = textureIndex.find(viewmodel.data);
				if (textureEntry == textureIndex.end())
				{
					auto newTexture = [Texture new];
					newTexture.descriptor = [MTLTextureDescriptor new];
					newTexture.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
					newTexture.descriptor.width = width;
					newTexture.descriptor.height = height;
					newTexture.texture = [device newTextureWithDescriptor:newTexture.descriptor];
					newTexture.region = MTLRegionMake2D(0, 0, width, height);
					
					[newTexture.texture replaceRegion:newTexture.region mipmapLevel:0 withBytes:(unsigned char*)viewmodel.data bytesPerRow:newTexture.region.size.width];

					textureIndex.insert({viewmodel.data, textureCache.count});
					
					texture.second.texture = (uint32_t)textureCache.count;
					
					[textureCache addObject:newTexture];
				}
				else
				{
					texture.second.texture = (uint32_t)textureEntry->second;
				}

				texture.second.set = true;
			}
			
			auto vertexSource = viewmodel.apverts;
			auto texCoordsSource = viewmodel.texture_coordinates;
			auto lightsSource = d_lists.colormapped_attributes.data() + viewmodel.first_attribute;
			for (auto i = 0; i < viewmodel.vertex_count; i++)
			{
				auto x = vertexSource->v[0];
				auto y = vertexSource->v[1];
				auto z = vertexSource->v[2];

				auto s = (float)(texCoordsSource->s >> 16);
				auto t = (float)(texCoordsSource->t >> 16);
				s /= width;
				t /= height;
				
				*vertices++ = x;
				*vertices++ = y;
				*vertices++ = z;
				*vertices++ = 1;
				*vertices++ = s;
				*vertices++ = t;
				*vertices++ = *lightsSource++;

				*vertices++ = x;
				*vertices++ = y;
				*vertices++ = z;
				*vertices++ = 1;
				*vertices++ = s + 0.5;
				*vertices++ = t;
				*vertices++ = *lightsSource++;

				vertexSource++;
				texCoordsSource++;
			}
			
			auto indicesEntry = aliasIndicesIndex.find(viewmodel.aliashdr);
			if (indicesEntry == aliasIndicesIndex.end())
			{
				auto aliashdr = (aliashdr_t *)viewmodel.aliashdr;
				auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
				auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
				auto stverts = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);

				auto newIndices = [device newBufferWithLength:entry.indices * sizeof(uint32_t) options:0];

				auto target = (uint32_t*)newIndices.contents;

				for (auto i = 0; i < mdl->numtris; i++)
				{
					auto v0 = triangle->vertindex[0];
					auto v1 = triangle->vertindex[1];
					auto v2 = triangle->vertindex[2];
					auto v0back = (((stverts[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
					auto v1back = (((stverts[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
					auto v2back = (((stverts[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
					*target++ = v0 * 2 + (v0back ? 1 : 0);
					*target++ = v1 * 2 + (v1back ? 1 : 0);
					*target++ = v2 * 2 + (v2back ? 1 : 0);
					triangle++;
				}

				aliasIndicesIndex.insert({viewmodel.aliashdr, aliasIndicesCache.count});
				
				entry.indexBuffer = (uint32_t)aliasIndicesCache.count;
				
				[aliasIndicesCache addObject:newIndices];
			}
			else
			{
				entry.indexBuffer = (uint32_t)indicesEntry->second;
			}

			if (viewmodel.colormap != nullptr)
			{
				auto colormapEntry = colormapIndex.find(viewmodel.colormap);
				if (colormapEntry == colormapIndex.end())
				{
					auto newColormap = [Texture new];
					newColormap.descriptor = [MTLTextureDescriptor new];
					newColormap.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
					newColormap.descriptor.width = 256;
					newColormap.descriptor.height = 64;
					newColormap.texture = [device newTextureWithDescriptor:perDrawable.colormap.descriptor];
					newColormap.region = MTLRegionMake2D(0, 0, 256, 64);
					
					[newColormap.texture replaceRegion:newColormap.region mipmapLevel:0 withBytes:(unsigned char*)viewmodel.colormap bytesPerRow:newColormap.region.size.width];

					colormapIndex.insert({viewmodel.colormap, colormapCache.count});
					
					entry.colormap = (uint32_t)textureCache.count;
					
					[colormapCache addObject:newColormap];
				}
				else
				{
					entry.colormap = (uint32_t)colormapEntry->second;
				}
			}
			else
			{
				entry.colormap = -1;
			}
		}
	}
}

void Viewmodel::Render(std::unordered_map<void*, SortedAliasTexture>& sorted, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger& vertexBase, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState, NSMutableArray<id<MTLBuffer>>* aliasIndicesCache, NSMutableArray<Texture*>* colormapCache)
{
	if (sorted.size() >= 0)
	{
		[commandEncoder setRenderPipelineState:pipeline];
		[commandEncoder setDepthStencilState:depthStencilState];
		[commandEncoder setVertexBytes:&vertexTransformMatrix length:sizeof(vertexTransformMatrix) atIndex:2];
		[commandEncoder setVertexBytes:&viewMatrix length:sizeof(viewMatrix) atIndex:3];
		[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:4];
		[commandEncoder setFragmentTexture:perDrawable.palette.texture atIndex:0];
		[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:0];

		int32_t previousColormap = -999999;
		
		for (auto& texture : sorted)
		{
			[commandEncoder setFragmentTexture:textureCache[texture.second.texture].texture atIndex:2];
			[commandEncoder setFragmentSamplerState:textureSamplerState atIndex:2];

			for (auto& entry : texture.second.entries)
			{
				[commandEncoder setVertexBuffer:perDrawable.aliasVertices offset:vertexBase atIndex:0];

				if (previousColormap != entry.colormap)
				{
					if (entry.colormap >= 0)
					{
						[commandEncoder setFragmentTexture:colormapCache[entry.colormap].texture atIndex:1];
					}
					else
					{
						[commandEncoder setFragmentTexture:perDrawable.colormap.texture atIndex:1];
					}
					[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:1];

					previousColormap = entry.colormap;
				}

				[commandEncoder setVertexBytes:entry.transform length:sizeof(entry.transform) atIndex:1];
				[commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:(NSUInteger)entry.indices indexType:MTLIndexTypeUInt32 indexBuffer:aliasIndicesCache[entry.indexBuffer] indexBufferOffset:0];
				
				vertexBase += entry.vertices * sizeof(float);
			}
		}
	}

}

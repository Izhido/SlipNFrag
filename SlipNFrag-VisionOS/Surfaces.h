//
//  Surfaces.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 25/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <unordered_map>
#include "SortedSurfaceLightmap.h"
#import <Metal/Metal.h>
#import "PerDrawable.h"

struct Surfaces
{
	static void Sort(std::unordered_map<void*, SortedSurfaceLightmap>& sorted, NSUInteger& verticesSize, NSUInteger& indicesSize);
	
	static void Fill(std::unordered_map<void*, SortedSurfaceLightmap>& sorted, float*& vertices, uint32_t*& indices, uint32_t& base, std::unordered_map<void*, NSUInteger>* lightmapIndex, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache);
	
	static void Render(std::unordered_map<void*, SortedSurfaceLightmap>& sorted, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger indexBase, id<MTLSamplerState> planarSamplerState, id<MTLSamplerState> lightmapSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState);
};

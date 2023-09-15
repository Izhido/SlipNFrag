//
//  Viewmodel.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 14/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <unordered_map>
#include "SortedAliasTexture.h"
#import <Metal/Metal.h>
#import "PerDrawable.h"

struct Viewmodel
{
	static void Sort(std::unordered_map<void*, SortedAliasTexture>& sorted, NSUInteger& verticesSize);
	
	static void Fill(std::unordered_map<void*, SortedAliasTexture>& sorted, float*& vertices, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache, std::unordered_map<void*, NSUInteger>& aliasIndicesIndex, NSMutableArray<id<MTLBuffer>>* aliasIndicesCache, std::unordered_map<void*, NSUInteger>& colormapIndex, NSMutableArray<Texture*>* colormapCache);
	
	static void Render(std::unordered_map<void*, SortedAliasTexture>& sorted, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger& vertexBase, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState, NSMutableArray<id<MTLBuffer>>* aliasIndicesCache, NSMutableArray<Texture*>* colormapCache);
};

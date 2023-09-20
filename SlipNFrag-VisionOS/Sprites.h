//
//  Sprites.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 18/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <unordered_map>
#include "SortedSpriteTexture.h"
#import <Metal/Metal.h>
#import "PerDrawable.h"

struct Sprites
{
	static void Sort(std::unordered_map<void*, SortedSpriteTexture>& sorted, NSUInteger& verticesSize, NSUInteger& indicesSize);
	
	static void Fill(std::unordered_map<void*, SortedSpriteTexture>& sorted, float*& vertices, uint32_t*& indices, uint32_t& base, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache);
	
	static void Render(std::unordered_map<void*, SortedSpriteTexture>& sorted, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger indexBase, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState);
};

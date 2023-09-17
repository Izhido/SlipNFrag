//
//  Sky.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 15/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include <vector>
#include <unordered_map>
#include "SkyEntry.h"
#import <Metal/Metal.h>
#import "PerDrawable.h"

struct Sky
{
	static void Sort(std::vector<SkyEntry>& skyEntries, NSUInteger& verticesSize, NSUInteger& indicesSize);
	
	static void Fill(std::vector<SkyEntry>& skyEntries, float*& vertices, uint32_t*& indices, id<MTLDevice> device, PerDrawable* perDrawable, std::vector<unsigned char>& skyBuffer, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache);
	
	static void Render(std::vector<SkyEntry>& skyEntries, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, simd_float4x4& headPose, cp_drawable_t& drawable, NSUInteger view, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState);
};

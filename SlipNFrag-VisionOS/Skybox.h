//
//  Skybox.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 1/10/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include <vector>
#include <unordered_map>
#include "SkyEntry.h"
#import <Metal/Metal.h>
#import "PerDrawable.h"

struct Skybox
{
	static void Sort(std::vector<SkyEntry>& skyboxEntries, NSUInteger& verticesSize, NSUInteger& indicesSize);
	
	static void Fill(std::vector<SkyEntry>& skyboxEntries, float*& vertices, uint32_t*& indices, id<MTLDevice> device, PerDrawable* perDrawable, std::unordered_map<void*, NSUInteger>& textureIndex, NSMutableArray<Texture*>* textureCache);
	
	static void Render(std::vector<SkyEntry>& skyboxEntries, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, simd_float4x4& headPose, cp_drawable_t& drawable, id<MTLSamplerState> planarSamplerState, NSMutableArray<Texture*>* textureCache, id<MTLSamplerState> textureSamplerState);
};

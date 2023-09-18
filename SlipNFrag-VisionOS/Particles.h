//
//  Particles.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 17/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include <vector>
#include "ParticlesEntry.h"
#import <Metal/Metal.h>
#import "PerDrawable.h"

struct Particles
{
	static void Sort(std::vector<ParticlesEntry>& particleEntries, NSUInteger& verticesSize, NSUInteger& indicesSize);
	
	static void Fill(std::vector<ParticlesEntry>& particleEntries, float*& vertices, uint32_t*& indices, id<MTLDevice> device, PerDrawable* perDrawable);
	
	static void Render(std::vector<ParticlesEntry>& particleEntries, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger indexBase, cp_drawable_t& drawable, id<MTLSamplerState> planarSamplerState);
};

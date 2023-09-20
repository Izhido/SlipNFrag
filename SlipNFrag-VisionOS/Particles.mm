//
//  Particles.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 17/9/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Particles.h"
#include "vid_visionos.h"
#include "d_lists.h"

void Particles::Sort(std::vector<ParticlesEntry>& particleEntries, NSUInteger& verticesSize, NSUInteger& indicesSize)
{
	if (d_lists.last_particle_color >= 0)
	{
		if (particleEntries.size() == 0)
		{
			particleEntries.emplace_back();
		}
		
		auto& entry = particleEntries.back();

		auto particles = d_lists.last_particle_color + 1;
		
		entry.indices += 6 * particles;

		verticesSize += particles * 4 * 5 * sizeof(float);
		indicesSize += particles * 6 * sizeof(uint32_t);
	}
}

void Particles::Fill(std::vector<ParticlesEntry>& particleEntries, float*& vertices, uint32_t*& indices, id<MTLDevice> device, PerDrawable* perDrawable)
{
	if (particleEntries.size() > 0)
	{
		auto verticesSource = d_lists.particle_positions.data();
		auto colorsSource = d_lists.particle_colors.data();
		
		uint32_t indexBase = 0;
		
		for (auto p = 0; p <= d_lists.last_particle_color; p++)
		{
			auto x = *verticesSource++;
			auto y = *verticesSource++;
			auto z = *verticesSource++;
			auto color = *colorsSource++;
			
			*vertices++ = x - 0.5 * d_lists.vright0 + 0.5 * d_lists.vup0;
			*vertices++ = y - 0.5 * d_lists.vright1 + 0.5 * d_lists.vup1;
			*vertices++ = z - 0.5 * d_lists.vright2 + 0.5 * d_lists.vup2;
			*vertices++ = 1;
			*vertices++ = color;
			
			*vertices++ = x + 0.5 * d_lists.vright0 + 0.5 * d_lists.vup0;
			*vertices++ = y + 0.5 * d_lists.vright1 + 0.5 * d_lists.vup1;
			*vertices++ = z + 0.5 * d_lists.vright2 + 0.5 * d_lists.vup2;
			*vertices++ = 1;
			*vertices++ = color;

			*vertices++ = x + 0.5 * d_lists.vright0 - 0.5 * d_lists.vup0;
			*vertices++ = y + 0.5 * d_lists.vright1 - 0.5 * d_lists.vup1;
			*vertices++ = z + 0.5 * d_lists.vright2 - 0.5 * d_lists.vup2;
			*vertices++ = 1;
			*vertices++ = color;

			*vertices++ = x - 0.5 * d_lists.vright0 - 0.5 * d_lists.vup0;
			*vertices++ = y - 0.5 * d_lists.vright1 - 0.5 * d_lists.vup1;
			*vertices++ = z - 0.5 * d_lists.vright2 - 0.5 * d_lists.vup2;
			*vertices++ = 1;
			*vertices++ = color;
			
			*indices++ = indexBase;
			*indices++ = indexBase + 1;
			*indices++ = indexBase + 2;
			*indices++ = indexBase + 2;
			*indices++ = indexBase + 3;
			*indices++ = indexBase;
			
			indexBase += 4;
		}
	}
}

void Particles::Render(std::vector<ParticlesEntry>& particleEntries, id<MTLRenderCommandEncoder> commandEncoder, id<MTLRenderPipelineState> pipeline, id<MTLDepthStencilState> depthStencilState, simd_float4x4& vertexTransformMatrix, simd_float4x4& viewMatrix, simd_float4x4& projectionMatrix, PerDrawable* perDrawable, NSUInteger indexBase, cp_drawable_t& drawable, id<MTLSamplerState> planarSamplerState)
{
	if (particleEntries.size() > 0)
	{
		[commandEncoder setRenderPipelineState:pipeline];
		[commandEncoder setDepthStencilState:depthStencilState];
		[commandEncoder setVertexBytes:&vertexTransformMatrix length:sizeof(vertexTransformMatrix) atIndex:1];
		[commandEncoder setVertexBytes:&viewMatrix length:sizeof(viewMatrix) atIndex:2];
		[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:3];
		[commandEncoder setFragmentTexture:perDrawable.palette.texture atIndex:0];
		[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:0];
		[commandEncoder setVertexBuffer:perDrawable.particleVertices offset:0 atIndex:0];

		for (auto& entry : particleEntries)
		{
			[commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:(NSUInteger)entry.indices indexType:MTLIndexTypeUInt32 indexBuffer:perDrawable.indices indexBufferOffset:indexBase];
				
			indexBase += (NSUInteger)entry.indices * sizeof(uint32_t);
		}
	}
}

//
//  Shaders.metal
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 24/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include <metal_stdlib>

using namespace metal;

struct VertexIn
{
	float4 position [[attribute(0)]];
	float2 texCoords [[attribute(1)]];
};

struct VertexOut
{
	float4 position [[position]];
	float2 texCoords;
};

struct SurfaceVertexIn
{
	float3 position [[attribute(0)]];
};

[[vertex]] VertexOut planarVertexMain(VertexIn inVertex [[stage_in]], constant float4x4& viewMatrix [[buffer(1)]], constant float4x4& projectionMatrix [[buffer(2)]])
{
	VertexOut outVertex;
	outVertex.position = projectionMatrix * viewMatrix * inVertex.position;
	outVertex.texCoords = inVertex.texCoords;
	return outVertex;
}

[[fragment]] half4 planarFragmentMain(VertexOut input [[stage_in]], texture2d<half> screenTexture [[texture(0)]], texture2d<half> consoleTexture [[texture(1)]], texture1d<float> paletteTexture [[texture(2)]], sampler screenSampler [[sampler(0)]], sampler consoleSampler [[sampler(1)]], sampler paletteSampler [[sampler(2)]])
{
	half consoleEntry = consoleTexture.sample(consoleSampler, input.texCoords)[0];
	half screenEntry = screenTexture.sample(screenSampler, input.texCoords)[0];
	return half4(paletteTexture.sample(paletteSampler, (consoleEntry < 1 ? consoleEntry : screenEntry)));
}

[[fragment]] half4 consoleFragmentMain(VertexOut input [[stage_in]], texture2d<half> consoleTexture [[texture(0)]], texture1d<float> paletteTexture [[texture(1)]], sampler consoleSampler [[sampler(0)]], sampler paletteSampler [[sampler(1)]])
{
	half entry = consoleTexture.sample(consoleSampler, input.texCoords)[0];
	return half4(paletteTexture.sample(paletteSampler, entry));
}

[[vertex]] VertexOut surfaceVertexMain(SurfaceVertexIn inVertex [[stage_in]], constant float4x4& vertexTransformMatrix [[buffer(1)]], constant float4x4& viewMatrix [[buffer(2)]], constant float4x4& projectionMatrix [[buffer(3)]], constant float2x4& vecs [[buffer(4)]], constant float2& size [[buffer(5)]])
{
	VertexOut outVertex;
	auto position = float4(inVertex.position, 1);
	outVertex.position = projectionMatrix * viewMatrix * vertexTransformMatrix * position;
	outVertex.texCoords = float2(dot(position, vecs[0]), dot(position, vecs[1])) / size;
	return outVertex;
}

[[fragment]] half4 surfaceFragmentMain(VertexOut input [[stage_in]], texture2d<half> surfaceTexture [[texture(0)]], texture1d<float> paletteTexture [[texture(1)]], sampler surfaceSampler [[sampler(0)]], sampler paletteSampler [[sampler(1)]])
{
	half entry = surfaceTexture.sample(surfaceSampler, input.texCoords)[0];
	return half4(paletteTexture.sample(paletteSampler, entry));
}

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

struct SurfaceVertexOut
{
	float4 position [[position]];
	float2 lightmapCoords;
	float2 texCoords;
	float2 lightmapSize;
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

[[vertex]] SurfaceVertexOut surfaceVertexMain(SurfaceVertexIn inVertex [[stage_in]], constant float4x4& vertexTransformMatrix [[buffer(1)]], constant float4x4& viewMatrix [[buffer(2)]], constant float4x4& projectionMatrix [[buffer(3)]], constant float4x4& texturePosition [[buffer(4)]])
{
	SurfaceVertexOut outVertex;
	auto position = float4(inVertex.position, 1);
	outVertex.position = projectionMatrix * viewMatrix * vertexTransformMatrix * position;
	auto texCoords = float2(dot(position, texturePosition[0]), dot(position, texturePosition[1]));
	auto mins = float2(texturePosition[2][0], texturePosition[2][1]);
	auto extents = float2(texturePosition[2][2], texturePosition[2][3]);
	auto size = float2(texturePosition[3][0], texturePosition[3][1]);
	outVertex.lightmapCoords = (texCoords - mins) / extents;
	outVertex.texCoords = texCoords / size;
	outVertex.lightmapSize = (floor(extents / 16) + 1) * 16;
	return outVertex;
}

[[fragment]] half4 surfaceFragmentMain(SurfaceVertexOut input [[stage_in]], texture1d<half> paletteTexture [[texture(0)]], texture2d<half> colormapTexture [[texture(1)]], texture2d<float> lightmapTexture [[texture(2)]], texture2d<half> surfaceTexture [[texture(3)]], sampler paletteSampler [[sampler(0)]], sampler colormapSampler [[sampler(1)]], sampler lightmapSampler [[sampler(2)]], sampler textureSampler [[sampler(3)]])
{
	auto lightmapCoords = floor(input.lightmapCoords * input.lightmapSize) / input.lightmapSize;
	auto light = lightmapTexture.sample(lightmapSampler, lightmapCoords)[0] * 4;
	auto textureEntry = surfaceTexture.sample(textureSampler, input.texCoords)[0];
	auto colormapCoords = float2(textureEntry, light);
	auto colormapped = colormapTexture.sample(colormapSampler, colormapCoords)[0];
	auto color = paletteTexture.sample(paletteSampler, colormapped);
	return color;
}

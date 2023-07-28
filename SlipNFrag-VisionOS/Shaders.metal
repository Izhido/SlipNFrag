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

[[vertex]] VertexOut vertexMain(VertexIn inVertex [[stage_in]], constant float4x4& viewMatrix [[buffer(1)]], constant float4x4& projectionMatrix [[buffer(2)]])
{
	VertexOut outVertex;
	outVertex.position = projectionMatrix * viewMatrix * inVertex.position;
	outVertex.texCoords = inVertex.texCoords;
	return outVertex;
}

[[fragment]] half4 fragmentMain(VertexOut input [[stage_in]], texture2d<half> screenTexture [[texture(0)]], texture2d<half> consoleTexture [[texture(1)]], texture1d<float> paletteTexture [[texture(2)]], sampler screenSampler [[sampler(0)]], sampler consoleSampler [[sampler(1)]], sampler paletteSampler [[sampler(2)]])
{
	half consoleEntry = consoleTexture.sample(consoleSampler, input.texCoords)[0];
	half screenEntry = screenTexture.sample(screenSampler, input.texCoords)[0];
	return half4(paletteTexture.sample(paletteSampler, (consoleEntry < 1 ? consoleEntry : screenEntry)));
}

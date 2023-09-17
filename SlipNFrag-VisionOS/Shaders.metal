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
	float4 position [[attribute(0)]];
};

struct SurfaceVertexOut
{
	float4 position [[position]];
	float2 lightmapCoords;
	float2 texCoords;
	float2 lightmapSize;
};

struct AliasVertexIn
{
	float4 position [[attribute(0)]];
	float2 texCoords [[attribute(1)]];
	float light [[attribute(2)]];
};

struct AliasVertexOut
{
	float4 position [[position]];
	float2 texCoords;
	float light;
};

struct ViewmodelVertexOut
{
	float4 position [[position]];
	float2 texCoords;
	float light;
	float alpha;
};

struct SkyRotation
{
	float forwardX;
	float forwardY;
	float forwardZ;
	float rightX;
	float rightY;
	float rightZ;
	float upX;
	float upY;
	float upZ;
	float width;
	float height;
	float maxSize;
	float speed;
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
	auto position = inVertex.position;
	outVertex.position = projectionMatrix * viewMatrix * vertexTransformMatrix * position;
	auto texCoords = float2(dot(position, texturePosition[0]), dot(position, texturePosition[1]));
	auto mins = texturePosition[2].xy;
	auto extents = texturePosition[2].zw;
	auto size = texturePosition[3].xy;
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

[[vertex]] SurfaceVertexOut surfaceRotatedVertexMain(SurfaceVertexIn inVertex [[stage_in]], constant float4x4& vertexTransformMatrix [[buffer(1)]], constant float4x4& viewMatrix [[buffer(2)]], constant float4x4& projectionMatrix [[buffer(3)]], constant float4x4& texturePosition [[buffer(4)]], constant float2x4& rotation [[buffer(5)]])
{
	SurfaceVertexOut outVertex;
	auto position = inVertex.position;
	auto translation = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, rotation[0].x, rotation[0].y, rotation[0].z, 1);
	auto sine = float3(sin(-rotation[1].x), sin(-rotation[1].y), sin(-rotation[1].z));
	auto cosine = float3(cos(-rotation[1].x), cos(-rotation[1].y), cos(-rotation[1].z));
	auto yawRotation = float4x4(cosine.y, 0, sine.y, 0, 0, 1, 0, 0, -sine.y, 0, cosine.y, 0, 0, 0, 0, 1);
	auto pitchRotation = float4x4(cosine.x, -sine.x, 0, 0, sine.x, cosine.x, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	auto rollRotation = float4x4(1, 0, 0, 0, 0, cosine.z, -sine.z, 0, 0, sine.z, cosine.z, 0, 0, 0, 0, 1);
	outVertex.position = projectionMatrix * viewMatrix * vertexTransformMatrix * translation * rollRotation * pitchRotation * yawRotation * position;
	auto texCoords = float2(dot(position, texturePosition[0]), dot(position, texturePosition[1]));
	auto mins = texturePosition[2].xy;
	auto extents = texturePosition[2].zw;
	auto size = texturePosition[3].xy;
	outVertex.lightmapCoords = (texCoords - mins) / extents;
	outVertex.texCoords = texCoords / size;
	outVertex.lightmapSize = (floor(extents / 16) + 1) * 16;
	return outVertex;
}

[[vertex]] VertexOut turbulentVertexMain(SurfaceVertexIn inVertex [[stage_in]], constant float4x4& vertexTransformMatrix [[buffer(1)]], constant float4x4& viewMatrix [[buffer(2)]], constant float4x4& projectionMatrix [[buffer(3)]], constant float2x4& texturePosition [[buffer(4)]], constant float2& textureSize [[buffer(5)]])
{
	VertexOut outVertex;
	auto position = inVertex.position;
	outVertex.position = projectionMatrix * viewMatrix * vertexTransformMatrix * position;
	auto texCoords = float2(dot(position, texturePosition[0]), dot(position, texturePosition[1]));
	outVertex.texCoords = texCoords / textureSize;
	return outVertex;
}

[[fragment]] half4 turbulentFragmentMain(VertexOut input [[stage_in]], constant float& time [[buffer(0)]], texture1d<half> paletteTexture [[texture(0)]], texture2d<half> surfaceTexture [[texture(1)]], sampler paletteSampler [[sampler(0)]], sampler textureSampler [[sampler(1)]])
{
	auto distortion = sin(fmod(time + input.texCoords * 5, 3.14159*2)) / 10;
	auto texCoords = input.texCoords.xy + distortion.yx;
	auto entry = surfaceTexture.sample(textureSampler, texCoords)[0];
	auto color = paletteTexture.sample(paletteSampler, entry);
	return color;
}

[[vertex]] AliasVertexOut aliasVertexMain(AliasVertexIn inVertex [[stage_in]], constant float4x4& aliasTransformMatrix  [[buffer(1)]], constant float4x4& vertexTransformMatrix [[buffer(2)]], constant float4x4& viewMatrix [[buffer(3)]], constant float4x4& projectionMatrix [[buffer(4)]])
{
	AliasVertexOut outVertex;
	outVertex.position = projectionMatrix * viewMatrix * vertexTransformMatrix * aliasTransformMatrix * inVertex.position;
	outVertex.texCoords = inVertex.texCoords;
	outVertex.light = inVertex.light / 64;
	return outVertex;
}

[[fragment]] half4 aliasFragmentMain(AliasVertexOut input [[stage_in]], texture1d<half> paletteTexture [[texture(0)]], texture2d<half> colormapTexture [[texture(1)]], texture2d<half> skinTexture [[texture(2)]], sampler paletteSampler [[sampler(0)]], sampler colormapSampler [[sampler(1)]], sampler skinSampler [[sampler(2)]])
{
	auto textureEntry = skinTexture.sample(skinSampler, input.texCoords)[0];
	auto colormapCoords = float2(textureEntry, input.light);
	auto colormapped = colormapTexture.sample(colormapSampler, colormapCoords)[0];
	auto color = paletteTexture.sample(paletteSampler, colormapped);
	return color;
}

[[vertex]] ViewmodelVertexOut viewmodelVertexMain(AliasVertexIn inVertex [[stage_in]], constant float4x4& aliasTransformMatrix  [[buffer(1)]], constant float4x4& vertexTransformMatrix [[buffer(2)]], constant float4x4& viewMatrix [[buffer(3)]], constant float4x4& projectionMatrix [[buffer(4)]])
{
	ViewmodelVertexOut outVertex;
	auto position = viewMatrix * vertexTransformMatrix * aliasTransformMatrix * inVertex.position;
	outVertex.position = projectionMatrix * position;
	outVertex.texCoords = inVertex.texCoords;
	outVertex.light = inVertex.light / 64;
	outVertex.alpha = clamp(-position.z * 9 - 2, 0.0, 1.0);
	return outVertex;
}

[[fragment]] half4 viewmodelFragmentMain(ViewmodelVertexOut input [[stage_in]], texture1d<half> paletteTexture [[texture(0)]], texture2d<half> colormapTexture [[texture(1)]], texture2d<half> skinTexture [[texture(2)]], sampler paletteSampler [[sampler(0)]], sampler colormapSampler [[sampler(1)]], sampler skinSampler [[sampler(2)]])
{
	auto textureEntry = skinTexture.sample(skinSampler, input.texCoords)[0];
	auto colormapCoords = float2(textureEntry, input.light);
	auto colormapped = colormapTexture.sample(colormapSampler, colormapCoords)[0];
	auto color = paletteTexture.sample(paletteSampler, colormapped);
	return color * half4(1, 1, 1, input.alpha);
}

[[vertex]] VertexOut skyVertexMain(VertexIn inVertex [[stage_in]], constant float4x4& projectionMatrix [[buffer(1)]])
{
	VertexOut outVertex;
	outVertex.position = projectionMatrix * inVertex.position;
	outVertex.texCoords = inVertex.texCoords;
	return outVertex;
}

[[fragment]] half4 skyFragmentMain(VertexOut input [[stage_in]], constant SkyRotation& rotation [[buffer(0)]], texture1d<half> paletteTexture [[texture(0)]], texture2d<half> surfaceTexture [[texture(1)]], sampler paletteSampler [[sampler(0)]], sampler textureSampler [[sampler(1)]])
{
	int u = int(input.texCoords.x * int(rotation.width));
	int v = int(input.texCoords.y * int(rotation.height));
	float	wu, wv, temp;
	float3	end;
	temp = rotation.maxSize;
	wu = 8192.0f * float(u-int(int(rotation.width)>>1)) / temp;
	wv = 8192.0f * float(int(int(rotation.height)>>1)-v) / temp;
	end[0] = 4096.0f*rotation.forwardX + wu*rotation.rightX + wv*rotation.upX;
	end[1] = 4096.0f*rotation.forwardY + wu*rotation.rightY + wv*rotation.upY;
	end[2] = 4096.0f*rotation.forwardZ + wu*rotation.rightZ + wv*rotation.upZ;
	end[2] *= 3;
	end = normalize(end);
	temp = rotation.speed;
	float s = float((temp + 6*(128/2-1)*end[0]));
	float t = float((temp + 6*(128/2-1)*end[1]));
	auto texCoords = float2(s / 128.0f, t / 128.0f);
	auto entry = surfaceTexture.sample(textureSampler, texCoords)[0];
	auto color = paletteTexture.sample(paletteSampler, entry);
	return color;
}

#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision mediump int;

layout(push_constant) uniform Lightmap
{
	layout(offset = 0) float lightmapIndex;
};

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 0, binding = 2) uniform usampler2D fragmentColormap;
layout(set = 1, binding = 0) uniform sampler2DArray fragmentLightmap;
layout(set = 2, binding = 0) uniform usampler2D fragmentTexture;

layout(location = 0) in vec4 fragmentData;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	ivec2 lightmapCoords = ivec2(floor(fragmentData.xy));
	vec4 lightmapTopLeftEntry = texelFetch(fragmentLightmap, ivec3(lightmapCoords, lightmapIndex), 0);
	vec4 lightmapTopRightEntry = texelFetch(fragmentLightmap, ivec3(lightmapCoords.x + 1, lightmapCoords.y, lightmapIndex), 0);
	vec4 lightmapBottomRightEntry = texelFetch(fragmentLightmap, ivec3(lightmapCoords.x + 1, lightmapCoords.y + 1, lightmapIndex), 0);
	vec4 lightmapBottomLeftEntry = texelFetch(fragmentLightmap, ivec3(lightmapCoords.x, lightmapCoords.y + 1, lightmapIndex), 0);
	vec2 lightmapCoordsDelta = floor(((fragmentData.xy - lightmapCoords) + 0.03125) * 16) / 16;
	float lightmapTopEntry = mix(lightmapTopLeftEntry.x, lightmapTopRightEntry.x, lightmapCoordsDelta.x);
	float lightmapBottomEntry = mix(lightmapBottomLeftEntry.x, lightmapBottomRightEntry.x, lightmapCoordsDelta.x);
	float lightmapEntry = mix(lightmapTopEntry, lightmapBottomEntry, lightmapCoordsDelta.y);
	uint light = (uint(lightmapEntry) / 256) % 64;
	vec2 texLevel = textureQueryLod(fragmentTexture, fragmentData.zw);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	uvec4 lowTexEntry = textureLod(fragmentTexture, fragmentData.zw, texMip.x);
	uvec4 highTexEntry = textureLod(fragmentTexture, fragmentData.zw, texMip.y);
	uvec4 lowColormapped = texelFetch(fragmentColormap, ivec2(lowTexEntry.x, light), 0);
	uvec4 highColormapped = texelFetch(fragmentColormap, ivec2(highTexEntry.x, light), 0);
	vec4 lowColor = palette[lowColormapped.x];
	vec4 highColor = palette[highColormapped.x];
	outColor = mix(lowColor, highColor, texLevel.y - texMip.x);
}

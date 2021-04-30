#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision mediump int;

layout(set = 0, binding = 1) uniform sampler2D fragmentPalette;
layout(set = 0, binding = 2) uniform usampler2D fragmentColormap;
layout(set = 1, binding = 0) uniform usampler2D fragmentTexture;
layout(set = 2, binding = 0) uniform sampler2D fragmentLightmap;

layout(location = 0) in vec2 fragmentLightmapCoords;
layout(location = 1) in vec2 fragmentTexCoords;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 texLevel = textureQueryLod(fragmentTexture, fragmentTexCoords);
	float lowTexMip = floor(texLevel.y);
	float highTexMip = ceil(texLevel.y);
	uvec4 lowTexEntry = textureLod(fragmentTexture, fragmentTexCoords, lowTexMip);
	uvec4 highTexEntry = textureLod(fragmentTexture, fragmentTexCoords, highTexMip);
	vec4 lightmapEntry = texture(fragmentLightmap, fragmentLightmapCoords);
	uint light = (uint(lightmapEntry.x) / 256) % 64;
	uvec4 lowColormapped = texelFetch(fragmentColormap, ivec2(lowTexEntry.x, light), 0);
	uvec4 highColormapped = texelFetch(fragmentColormap, ivec2(highTexEntry.x, light), 0);
	vec4 lowColor = texelFetch(fragmentPalette, ivec2(lowColormapped.x, 0), 0);
	vec4 highColor = texelFetch(fragmentPalette, ivec2(highColormapped.x, 0), 0);
	float delta = texLevel.y - lowTexMip;
	float r = lowColor.x + delta * (highColor.x - lowColor.x);
	float g = lowColor.y + delta * (highColor.y - lowColor.y);
	float b = lowColor.z + delta * (highColor.z - lowColor.z);
	float a = lowColor.w + delta * (highColor.w - lowColor.w);
	outColor = vec4(r, g, b, a);
}

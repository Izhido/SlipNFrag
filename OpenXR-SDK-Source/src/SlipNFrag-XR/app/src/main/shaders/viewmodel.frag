#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision mediump float;
precision mediump int;

layout(set = 0, binding = 1) uniform sampler2D fragmentPalette;
layout(set = 1, binding = 0) uniform usampler2D fragmentColormap;
layout(set = 2, binding = 0) uniform usampler2D fragmentTexture;

layout(push_constant) uniform Tint
{
	layout(offset = 80) float tintR;
	layout(offset = 84) float tintG;
	layout(offset = 88) float tintB;
	layout(offset = 92) float tintA;
};

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 1) in float fragmentLight;
layout(location = 2) in float fragmentAlpha;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 level = textureQueryLod(fragmentTexture, fragmentTexCoords);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	uvec4 lowEntry = textureLod(fragmentTexture, fragmentTexCoords, mip.x);
	uvec4 highEntry = textureLod(fragmentTexture, fragmentTexCoords, mip.y);
	uvec4 lowColormapped = texelFetch(fragmentColormap, ivec2(lowEntry.x, fragmentLight), 0);
	uvec4 highColormapped = texelFetch(fragmentColormap, ivec2(highEntry.x, fragmentLight), 0);
	vec4 lowColor = texelFetch(fragmentPalette, ivec2(lowColormapped.x, 0), 0);
	vec4 highColor = texelFetch(fragmentPalette, ivec2(highColormapped.x, 0), 0);
	vec4 color = mix(lowColor, highColor, level.y - mip.x);
	outColor = vec4(color.x * tintR, color.y * tintG, color.z * tintB, color.w * tintA * fragmentAlpha);
}

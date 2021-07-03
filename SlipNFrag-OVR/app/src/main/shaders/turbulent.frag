#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 0, binding = 1) uniform sampler2D fragmentPalette;
layout(set = 1, binding = 0) uniform usampler2D fragmentTexture;

layout(push_constant) uniform Time
{
	layout(offset = 64) float time;
};

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 distortion = sin(mod(time + fragmentTexCoords * 5, 3.14159*2)) / 10;
	vec2 texCoords = vec2(fragmentTexCoords.x + distortion.y, fragmentTexCoords.y + distortion.x);
	vec2 level = textureQueryLod(fragmentTexture, texCoords);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	uvec4 lowEntry = textureLod(fragmentTexture, texCoords, mip.x);
	uvec4 highEntry = textureLod(fragmentTexture, texCoords, mip.y);
	vec4 lowColor = texelFetch(fragmentPalette, ivec2(lowEntry.x, 0), 0);
	vec4 highColor = texelFetch(fragmentPalette, ivec2(highEntry.x, 0), 0);
	outColor = mix(lowColor, highColor, level.y - mip.x);
}

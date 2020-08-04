#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision mediump float;
precision mediump int;

layout(binding = 1) uniform sampler2D fragmentTexture;
layout(binding = 2) uniform sampler2D fragmentPalette;
layout(binding = 3) uniform Time
{
	layout(offset = 0) float time;
};

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	float t = mod(time, 3.14159 * 2);
	float tx = fragmentTexCoords.x + sin(t + fragmentTexCoords.y * 5) / 10;
	float ty = fragmentTexCoords.y + sin(t + fragmentTexCoords.x * 5) / 10;
	vec2 texCoords = vec2(tx, ty);
	vec2 level = textureQueryLod(fragmentTexture, texCoords);
	float lowMip = floor(level.y);
	float highMip = ceil(level.y);
	vec4 lowEntry = textureLod(fragmentTexture, texCoords, lowMip);
	vec4 highEntry = textureLod(fragmentTexture, texCoords, highMip);
	vec4 lowColor = texelFetch(fragmentPalette, ivec2(lowEntry.x * 255.0, 0), 0);
	vec4 highColor = texelFetch(fragmentPalette, ivec2(highEntry.x * 255.0, 0), 0);
	float delta = level.y - lowMip;
	float r = lowColor.x + delta * (highColor.x - lowColor.x);
	float g = lowColor.y + delta * (highColor.y - lowColor.y);
	float b = lowColor.z + delta * (highColor.z - lowColor.z);
	float a = lowColor.w + delta * (highColor.w - lowColor.w);
	outColor = vec4(r, g, b, a);
}

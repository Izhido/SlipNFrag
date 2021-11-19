#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision mediump float;
precision mediump int;

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 1, binding = 0) uniform usampler2D fragmentTexture;

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 level = textureQueryLod(fragmentTexture, fragmentTexCoords);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	uvec4 lowEntry = textureLod(fragmentTexture, fragmentTexCoords, mip.x);
	uvec4 highEntry = textureLod(fragmentTexture, fragmentTexCoords, mip.y);
	if (lowEntry.x == 255 || highEntry.x == 255)
	{
		discard;
	}
	vec4 lowColor = palette[lowEntry.x];
	vec4 highColor = palette[highEntry.x];
	outColor = mix(lowColor, highColor, level.y - mip.x);
}

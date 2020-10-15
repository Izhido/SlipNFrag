#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision mediump float;
precision mediump int;

layout(set = 0, binding = 1) uniform sampler2D fragmentPalette;
layout(set = 1, binding = 0) uniform sampler2D fragmentTexture;
layout(set = 1, binding = 1) uniform sampler2D fragmentColormap;

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 1) in float fragmentLight;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 level = textureQueryLod(fragmentTexture, fragmentTexCoords);
	float lowMip = floor(level.y);
	float highMip = ceil(level.y);
	vec4 lowEntry = textureLod(fragmentTexture, fragmentTexCoords, lowMip);
	vec4 highEntry = textureLod(fragmentTexture, fragmentTexCoords, highMip);
	vec4 lowColormapped = texelFetch(fragmentColormap, ivec2(lowEntry.x * 255.0, fragmentLight), 0);
	vec4 highColormapped = texelFetch(fragmentColormap, ivec2(highEntry.x * 255.0, fragmentLight), 0);
	vec4 lowColor = texelFetch(fragmentPalette, ivec2(lowColormapped.x * 255.0, 0), 0);
	vec4 highColor = texelFetch(fragmentPalette, ivec2(highColormapped.x * 255.0, 0), 0);
	float delta = level.y - lowMip;
	float r = lowColor.x + delta * (highColor.x - lowColor.x);
	float g = lowColor.y + delta * (highColor.y - lowColor.y);
	float b = lowColor.z + delta * (highColor.z - lowColor.z);
	float a = lowColor.w + delta * (highColor.w - lowColor.w);
	outColor = vec4(r, g, b, a);
}

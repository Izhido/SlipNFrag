#version 460

precision highp float;
precision highp int;

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 1, binding = 0) uniform usampler2DArray fragmentTexture;

layout(push_constant) uniform Time
{
	layout(offset = 0) float time;
};

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 1) in flat int fragmentTextureIndex;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 distortion = sin(mod(time + fragmentTexCoords * 5, 3.14159*2)) / 10;
	vec2 texCoords = fragmentTexCoords.xy + distortion.yx;
	vec2 level = textureQueryLod(fragmentTexture, texCoords);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	vec3 fragmentTextureCoords = vec3(texCoords, fragmentTextureIndex);
	uvec4 lowEntry = textureLod(fragmentTexture, fragmentTextureCoords, mip.x);
	uvec4 highEntry = textureLod(fragmentTexture, fragmentTextureCoords, mip.y);
	vec4 lowColor = palette[lowEntry.x];
	vec4 highColor = palette[highEntry.x];
	outColor = mix(lowColor, highColor, level.y - mip.x);
}

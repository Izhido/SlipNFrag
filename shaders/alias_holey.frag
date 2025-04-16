#version 460

precision mediump float;
precision mediump int;

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 1, binding = 0) uniform usampler2D fragmentColormap;
layout(set = 2, binding = 0) uniform usampler2D fragmentTexture;

layout(location = 0) in vec3 fragmentData;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 level = textureQueryLod(fragmentTexture, fragmentData.xy);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	uvec4 lowEntry = textureLod(fragmentTexture, fragmentData.xy, mip.x);
	uvec4 highEntry = textureLod(fragmentTexture, fragmentData.xy, mip.y);
	if (lowEntry.x == 255 || highEntry.x == 255)
	{
		discard;
	}
	uvec4 lowColormapped = texelFetch(fragmentColormap, ivec2(lowEntry.x, fragmentData.z), 0);
	uvec4 highColormapped = texelFetch(fragmentColormap, ivec2(highEntry.x, fragmentData.z), 0);
	vec4 lowColor = palette[lowColormapped.x];
	vec4 highColor = palette[highColormapped.x];
	outColor = mix(lowColor, highColor, level.y - mip.x);
}

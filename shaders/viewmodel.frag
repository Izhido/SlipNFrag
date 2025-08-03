#version 460

precision mediump float;
precision mediump int;

const float ditherThresholds[16] = float[]
(
	1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
	13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
	4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
	16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
);

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 1, binding = 0) uniform usampler2D fragmentColormap;
layout(set = 2, binding = 0) uniform usampler2D fragmentTexture;

layout(location = 0) in vec3 fragmentCoords;
layout(location = 1) in float fragmentLight;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	ivec2 ditherIndex = ivec2(gl_FragCoord.xy) % 4;
	if (fragmentCoords.z < ditherThresholds[ditherIndex.y * 4 + ditherIndex.x])
	{
		discard;
	}
	vec2 level = textureQueryLod(fragmentTexture, fragmentCoords.xy);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	uvec4 lowEntry = textureLod(fragmentTexture, fragmentCoords.xy, mip.x);
	uvec4 highEntry = textureLod(fragmentTexture, fragmentCoords.xy, mip.y);
	uvec4 lowColormapped = texelFetch(fragmentColormap, ivec2(lowEntry.x, fragmentLight), 0);
	uvec4 highColormapped = texelFetch(fragmentColormap, ivec2(highEntry.x, fragmentLight), 0);
	vec4 lowColor = palette[lowColormapped.x];
	vec4 highColor = palette[highColormapped.x];
	outColor = mix(lowColor, highColor, level.y - mip.x);
}

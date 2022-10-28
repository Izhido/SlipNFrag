#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 1, binding = 0) uniform usampler2DArray fragmentLightmap;
layout(set = 2, binding = 0) uniform usampler2DArray fragmentTexture;

layout(location = 0) in vec4 fragmentCoords;
layout(location = 1) in flat highp ivec2 fragmentTextureIndices;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	ivec3 lightmapCoords = ivec3(floor(fragmentCoords.xy), fragmentTextureIndices.x);
	vec4 lightmapTopLeftEntry = texelFetch(fragmentLightmap, lightmapCoords, 0);
	vec4 lightmapTopRightEntry = texelFetch(fragmentLightmap, lightmapCoords + ivec3(1, 0, 0), 0);
	vec4 lightmapBottomRightEntry = texelFetch(fragmentLightmap, lightmapCoords + ivec3(1, 1, 0), 0);
	vec4 lightmapBottomLeftEntry = texelFetch(fragmentLightmap, lightmapCoords + ivec3(0, 1, 0), 0);
	vec2 lightmapCoordsDelta = floor((fragmentCoords.xy - lightmapCoords.xy) * 16) / 16;
	vec4 lightmapTopEntry = mix(lightmapTopLeftEntry, lightmapTopRightEntry, lightmapCoordsDelta.x);
	vec4 lightmapBottomEntry = mix(lightmapBottomLeftEntry, lightmapBottomRightEntry, lightmapCoordsDelta.x);
	vec4 lightmapEntry = mix(lightmapTopEntry, lightmapBottomEntry, lightmapCoordsDelta.y);
	vec4 light = lightmapEntry / (128 * 256);
	vec2 texLevel = textureQueryLod(fragmentTexture, fragmentCoords.zw);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	vec3 fragmentTextureCoords = vec3(fragmentCoords.zw, fragmentTextureIndices.y);
	uvec4 lowTexEntry = textureLod(fragmentTexture, fragmentTextureCoords, texMip.x);
	uvec4 highTexEntry = textureLod(fragmentTexture, fragmentTextureCoords, texMip.y);
	vec4 lowColor = palette[lowTexEntry.x];
	vec4 highColor = palette[highTexEntry.x];
	vec4 color = mix(lowColor, highColor, texLevel.y - texMip.x) * light;
	outColor = vec4(
		((color.r < 0.04045) ? (color.r / 12.92) : (pow((color.r + 0.055) / 1.055, 2.4))),
		((color.g < 0.04045) ? (color.g / 12.92) : (pow((color.g + 0.055) / 1.055, 2.4))),
		((color.b < 0.04045) ? (color.b / 12.92) : (pow((color.b + 0.055) / 1.055, 2.4))),
		color.a
	);
}

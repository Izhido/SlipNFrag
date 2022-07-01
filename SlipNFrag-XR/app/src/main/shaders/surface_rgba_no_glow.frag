#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision mediump int;

layout(set = 1, binding = 0) uniform sampler2DArray fragmentLightmap;
layout(set = 2, binding = 0) uniform usampler2DArray fragmentTexture;

layout(push_constant) uniform Tint
{
	layout(offset = 0) vec4 tint;
};

layout(location = 0) in vec4 fragmentCoords;
layout(location = 1) in flat highp ivec2 fragmentTextureIndices;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	ivec2 lightmapCoords = ivec2(floor(fragmentCoords.xy));
	vec4 lightmapTopLeftEntry = texelFetch(fragmentLightmap, ivec3(lightmapCoords, fragmentTextureIndices.x), 0);
	vec4 lightmapTopRightEntry = texelFetch(fragmentLightmap, ivec3(lightmapCoords.x + 1, lightmapCoords.y, fragmentTextureIndices.x), 0);
	vec4 lightmapBottomRightEntry = texelFetch(fragmentLightmap, ivec3(lightmapCoords.x + 1, lightmapCoords.y + 1, fragmentTextureIndices.x), 0);
	vec4 lightmapBottomLeftEntry = texelFetch(fragmentLightmap, ivec3(lightmapCoords.x, lightmapCoords.y + 1, fragmentTextureIndices.x), 0);
	vec2 lightmapCoordsDelta = floor(((fragmentCoords.xy - lightmapCoords) + 0.03125) * 16) / 16;
	float lightmapTopEntry = mix(lightmapTopLeftEntry.x, lightmapTopRightEntry.x, lightmapCoordsDelta.x);
	float lightmapBottomEntry = mix(lightmapBottomLeftEntry.x, lightmapBottomRightEntry.x, lightmapCoordsDelta.x);
	float lightmapEntry = mix(lightmapTopEntry, lightmapBottomEntry, lightmapCoordsDelta.y);
	float light = 1 - clamp(lightmapEntry / (64 * 256), 0, 1);
	float gammaCorrected = ((light < 0.04045) ? (light / 12.92) : (pow((light + 0.055) / 1.055, 2.4))) / 256;
	vec2 texLevel = textureQueryLod(fragmentTexture, fragmentCoords.zw);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	vec3 fragmentTextureCoords = vec3(fragmentCoords.zw, fragmentTextureIndices.y);
	vec4 lowColor = textureLod(fragmentTexture, fragmentTextureCoords, texMip.x);
	vec4 highColor = textureLod(fragmentTexture, fragmentTextureCoords, texMip.y);
	outColor = mix(lowColor, highColor, texLevel.y - texMip.x) * vec4(gammaCorrected, gammaCorrected, gammaCorrected, 1) + tint;
}

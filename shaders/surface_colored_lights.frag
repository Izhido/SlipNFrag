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

layout(push_constant) uniform Tint
{
	layout(offset = 0) vec4 tint;
	layout(offset = 16) float gamma;
};

layout(location = 0) in vec4 fragmentCoords;
layout(location = 1) in flat highp ivec4 fragmentFlat;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	ivec3 lightmapCoords = ivec3(floor(clamp(fragmentCoords.xy, ivec2(0, 0), fragmentFlat.zw)), fragmentFlat.x);
	vec4 lightmapTopLeftEntry = texelFetch(fragmentLightmap, lightmapCoords, 0);
	vec4 lightmapTopRightEntry = texelFetchOffset(fragmentLightmap, lightmapCoords, 0, ivec2(1, 0));
	vec4 lightmapBottomRightEntry = texelFetchOffset(fragmentLightmap, lightmapCoords, 0, ivec2(1, 1));
	vec4 lightmapBottomLeftEntry = texelFetchOffset(fragmentLightmap, lightmapCoords, 0, ivec2(0, 1));
	vec2 lightmapCoordsDelta = floor((fragmentCoords.xy - lightmapCoords.xy) * 16) / 16;
	vec4 lightmapTopEntry = mix(lightmapTopLeftEntry, lightmapTopRightEntry, lightmapCoordsDelta.x);
	vec4 lightmapBottomEntry = mix(lightmapBottomLeftEntry, lightmapBottomRightEntry, lightmapCoordsDelta.x);
	vec4 lightmapEntry = mix(lightmapTopEntry, lightmapBottomEntry, lightmapCoordsDelta.y);
	vec4 light = lightmapEntry / 128;
	vec2 texLevel = textureQueryLod(fragmentTexture, fragmentCoords.zw);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	vec3 fragmentTextureCoords = vec3(fragmentCoords.zw, fragmentFlat.y);
	uvec4 lowTexEntry = textureLod(fragmentTexture, fragmentTextureCoords, texMip.x);
	uvec4 highTexEntry = textureLod(fragmentTexture, fragmentTextureCoords, texMip.y);
	vec4 lowColor = palette[lowTexEntry.x];
	vec4 highColor = palette[highTexEntry.x];
	vec4 color =
		mix(lowColor, highColor, texLevel.y - texMip.x) *
		((lowTexEntry.x >= 224 || highTexEntry.x >= 224) ? vec4(255, 255, 255, 255) : light);
	vec4 tinted = mix(color, tint, tint.a);
	vec4 gammaCorrected = vec4(
		clamp((gamma == 1) ? tinted.r : (255 * pow ( (tinted.r+0.5)/255.5 , gamma ) + 0.5), 0, 255),
		clamp((gamma == 1) ? tinted.g : (255 * pow ( (tinted.g+0.5)/255.5 , gamma ) + 0.5), 0, 255),
		clamp((gamma == 1) ? tinted.b : (255 * pow ( (tinted.b+0.5)/255.5 , gamma ) + 0.5), 0, 255),
		255
	) / 255;
	outColor = vec4(
		((gammaCorrected.r < 0.04045) ? (gammaCorrected.r / 12.92) : (pow((gammaCorrected.r + 0.055) / 1.055, 2.4))),
		((gammaCorrected.g < 0.04045) ? (gammaCorrected.g / 12.92) : (pow((gammaCorrected.g + 0.055) / 1.055, 2.4))),
		((gammaCorrected.b < 0.04045) ? (gammaCorrected.b / 12.92) : (pow((gammaCorrected.b + 0.055) / 1.055, 2.4))),
		gammaCorrected.a
	);
}

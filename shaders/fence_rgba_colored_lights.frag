#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 1, binding = 0) uniform usampler2DArray fragmentLightmap;
layout(set = 2, binding = 0) uniform usampler2DArray fragmentTexture;
layout(set = 3, binding = 0) uniform usampler2DArray fragmentGlowTexture;

layout(push_constant) uniform Tint
{
	layout(offset = 0) vec4 tint;
	layout(offset = 16) float gamma;
};

layout(location = 0) in vec4 fragmentCoords;
layout(location = 1) in flat highp ivec3 fragmentIndices;
layout(location = 2) in flat highp ivec2 fragmentSizes;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	ivec3 lightmapCoords = ivec3(floor(clamp(fragmentCoords.xy, ivec2(0, 0), fragmentSizes)), fragmentIndices.x);
	vec4 lightmapTopLeftEntry = texelFetch(fragmentLightmap, lightmapCoords, 0);
	vec4 lightmapTopRightEntry = texelFetchOffset(fragmentLightmap, lightmapCoords, 0, ivec2(1, 0));
	vec4 lightmapBottomRightEntry = texelFetchOffset(fragmentLightmap, lightmapCoords, 0, ivec2(1, 1));
	vec4 lightmapBottomLeftEntry = texelFetchOffset(fragmentLightmap, lightmapCoords, 0, ivec2(0, 1));
	vec2 lightmapCoordsDelta = fragmentCoords.xy - lightmapCoords.xy;
	vec4 lightmapTopEntry = mix(lightmapTopLeftEntry, lightmapTopRightEntry, lightmapCoordsDelta.x);
	vec4 lightmapBottomEntry = mix(lightmapBottomLeftEntry, lightmapBottomRightEntry, lightmapCoordsDelta.x);
	vec4 lightmapEntry = mix(lightmapTopEntry, lightmapBottomEntry, lightmapCoordsDelta.y);
	vec4 light = lightmapEntry / (128 * 256);
	vec2 texLevel = textureQueryLod(fragmentTexture, fragmentCoords.zw);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	vec3 fragmentTextureCoords = vec3(fragmentCoords.zw, fragmentIndices.y);
	vec4 lowColor = textureLod(fragmentTexture, fragmentTextureCoords, texMip.x);
	vec4 highColor = textureLod(fragmentTexture, fragmentTextureCoords, texMip.y);
	vec3 fragmentGlowTextureCoords = vec3(fragmentCoords.zw, fragmentIndices.z);
	vec4 lowGlow = textureLod(fragmentGlowTexture, fragmentGlowTextureCoords, texMip.x);
	vec4 highGlow = textureLod(fragmentGlowTexture, fragmentGlowTextureCoords, texMip.y);
	vec4 glow = mix(lowGlow, highGlow, texLevel.y - texMip.x);
	vec4 color = mix(lowColor, highColor, texLevel.y - texMip.x) * light + 2 * glow;
	if (color.a < 1.0 - (1.0 / 512.0))
	{
		discard;
	}
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

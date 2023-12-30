#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 1, binding = 0) uniform usampler2DArray fragmentLightmap;
layout(set = 2, binding = 0) uniform usampler2DArray fragmentTexture;

layout(push_constant) uniform Turbulent
{
	layout(offset = 0) vec4 tint;
	layout(offset = 16) float gamma;
	layout(offset = 20) float time;
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
	vec2 lightmapCoordsDelta = fragmentCoords.xy - lightmapCoords.xy;
	float lightmapTopEntry = mix(lightmapTopLeftEntry.x, lightmapTopRightEntry.x, lightmapCoordsDelta.x);
	float lightmapBottomEntry = mix(lightmapBottomLeftEntry.x, lightmapBottomRightEntry.x, lightmapCoordsDelta.x);
	float lightmapEntry = mix(lightmapTopEntry, lightmapBottomEntry, lightmapCoordsDelta.y);
	float light = 2 - lightmapEntry / (32 * 256);
	vec2 distortion = sin(mod(time + fragmentCoords.zw * 5, 3.14159*2)) / 10;
	vec2 texCoords = fragmentCoords.zw + distortion.yx;
	vec2 texLevel = textureQueryLod(fragmentTexture, texCoords);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	vec3 fragmentTextureCoords = vec3(texCoords, fragmentFlat.y);
	vec4 lowColor = textureLod(fragmentTexture, fragmentTextureCoords, texMip.x);
	vec4 highColor = textureLod(fragmentTexture, fragmentTextureCoords, texMip.y);
	vec4 color = mix(lowColor, highColor, texLevel.y - texMip.x) * vec4(light, light, light, 1);
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

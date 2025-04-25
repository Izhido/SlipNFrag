#version 460

precision highp float;
precision highp int;

const float ditherThresholds[16] = float[]
(
	1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
	13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
	4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
	16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
);

layout(set = 2, binding = 0) uniform usampler2DArray fragmentTexture;

layout(set = 3, binding = 0) readonly buffer FragmentLightmap
{
	uint lightmap[];
};

layout(push_constant) uniform Turbulent
{
	layout(offset = 0) vec4 tint;
	layout(offset = 16) float gamma;
	layout(offset = 20) float time;
};

layout(location = 0) in vec4 fragmentCoords;
layout(location = 1) in flat highp ivec4 fragmentFlat;
layout(location = 2) in flat int fragmentAlpha;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	if (fragmentAlpha > 0)
	{
		ivec2 ditherIndex = ivec2(gl_FragCoord.xy) % 4;
		if (float(fragmentAlpha) / 255 < ditherThresholds[ditherIndex.y * 4 + ditherIndex.x])
		{
			discard;
		}
	}
	vec2 lightmapClamped = floor(clamp(fragmentCoords.xy, ivec2(0, 0), fragmentFlat.zw));
	uint lightmapWidth = fragmentFlat.z + 1;
	uint lightmapTopIndex = fragmentFlat.x + int(lightmapClamped.y) * lightmapWidth + int(lightmapClamped.x);
	uint lightmapBottomIndex = lightmapTopIndex + lightmapWidth;
	float lightmapTopLeftEntry = float(lightmap[lightmapTopIndex]);
	float lightmapTopRightEntry = float(lightmap[lightmapTopIndex + 1]);
	float lightmapBottomRightEntry = float(lightmap[lightmapBottomIndex + 1]);
	float lightmapBottomLeftEntry = float(lightmap[lightmapBottomIndex]);
	vec2 lightmapCoordsDelta = fragmentCoords.xy - lightmapClamped;
	float lightmapTopEntry = mix(lightmapTopLeftEntry, lightmapTopRightEntry, lightmapCoordsDelta.x);
	float lightmapBottomEntry = mix(lightmapBottomLeftEntry, lightmapBottomRightEntry, lightmapCoordsDelta.x);
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

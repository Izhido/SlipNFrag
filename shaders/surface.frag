#version 460

precision highp float;
precision highp int;

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 0, binding = 2) uniform usampler2D fragmentColormap;
layout(set = 2, binding = 0) uniform usampler2DArray fragmentTexture;

layout(set = 3, binding = 0) readonly buffer FragmentLightmap
{
	uint lightmap[];
};

layout(location = 0) in vec4 fragmentCoords;
layout(location = 1) in flat highp ivec4 fragmentFlat;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 lightmapClamped = floor(clamp(fragmentCoords.xy, ivec2(0, 0), fragmentFlat.zw));
	uint lightmapWidth = fragmentFlat.z + 1;
	uint lightmapTopIndex = fragmentFlat.x + int(lightmapClamped.y) * lightmapWidth + int(lightmapClamped.x);
	uint lightmapBottomIndex = lightmapTopIndex + lightmapWidth;
	float lightmapTopLeftEntry = float(lightmap[lightmapTopIndex]);
	float lightmapTopRightEntry = float(lightmap[lightmapTopIndex + 1]);
	float lightmapBottomRightEntry = float(lightmap[lightmapBottomIndex + 1]);
	float lightmapBottomLeftEntry = float(lightmap[lightmapBottomIndex]);
	vec2 lightmapCoordsDelta = floor((fragmentCoords.xy - lightmapClamped) * 16) / 16;
	float lightmapTopEntry = mix(lightmapTopLeftEntry, lightmapTopRightEntry, lightmapCoordsDelta.x);
	float lightmapBottomEntry = mix(lightmapBottomLeftEntry, lightmapBottomRightEntry, lightmapCoordsDelta.x);
	float lightmapEntry = mix(lightmapTopEntry, lightmapBottomEntry, lightmapCoordsDelta.y);
	float light = lightmapEntry / 256;
	float lightBase = floor(light);
	float lightFrac = light - lightBase;
	uint lightBaseInt = uint(lightBase) % 64;
	uint lightNextInt = min(lightBaseInt + 1, 63);
	vec2 texLevel = textureQueryLod(fragmentTexture, fragmentCoords.zw);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	vec3 fragmentTextureCoords = vec3(fragmentCoords.zw, fragmentFlat.y);
	uvec4 lowTexEntry = textureLod(fragmentTexture, fragmentTextureCoords, texMip.x);
	uvec4 highTexEntry = textureLod(fragmentTexture, fragmentTextureCoords, texMip.y);
	uvec4 lowColormapped = texelFetch(fragmentColormap, ivec2(lowTexEntry.x, lightBaseInt), 0);
	uvec4 lowNextColormapped = texelFetch(fragmentColormap, ivec2(lowTexEntry.x, lightNextInt), 0);
	uvec4 highColormapped = texelFetch(fragmentColormap, ivec2(highTexEntry.x, lightBaseInt), 0);
	uvec4 highNextColormapped = texelFetch(fragmentColormap, ivec2(highTexEntry.x, lightNextInt), 0);
	vec4 lowColor = palette[lowColormapped.x];
	vec4 lowNextColor = palette[lowNextColormapped.x];
	vec4 highColor = palette[highColormapped.x];
	vec4 highNextColor = palette[highNextColormapped.x];
	outColor = mix(mix(lowColor, lowNextColor, lightFrac), mix(highColor, highNextColor, lightFrac), texLevel.y - texMip.x);
}

#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 0, binding = 2) uniform usampler2D fragmentColormap;
layout(set = 1, binding = 0) uniform usampler2DArray fragmentLightmap;
layout(set = 2, binding = 0) uniform usampler2DArray fragmentTexture;

layout(push_constant) uniform Time
{
	layout(offset = 0) float time;
};

layout(location = 0) in vec4 fragmentData;
layout(location = 1) in flat highp ivec2 fragmentTextureIndices;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	ivec3 lightmapCoords = ivec3(floor(fragmentData.xy), fragmentTextureIndices.x);
	vec4 lightmapTopLeftEntry = texelFetch(fragmentLightmap, lightmapCoords, 0);
	vec4 lightmapTopRightEntry = texelFetch(fragmentLightmap, lightmapCoords + ivec3(1, 0, 0), 0);
	vec4 lightmapBottomRightEntry = texelFetch(fragmentLightmap, lightmapCoords + ivec3(1, 1, 0), 0);
	vec4 lightmapBottomLeftEntry = texelFetch(fragmentLightmap, lightmapCoords + ivec3(0, 1, 0), 0);
	vec2 lightmapCoordsDelta = floor((fragmentData.xy - lightmapCoords.xy) * 16) / 16;
	float lightmapTopEntry = mix(lightmapTopLeftEntry.x, lightmapTopRightEntry.x, lightmapCoordsDelta.x);
	float lightmapBottomEntry = mix(lightmapBottomLeftEntry.x, lightmapBottomRightEntry.x, lightmapCoordsDelta.x);
	float lightmapEntry = mix(lightmapTopEntry, lightmapBottomEntry, lightmapCoordsDelta.y);
	float light = lightmapEntry / 256;
	float lightBase = floor(light);
	float lightFrac = light - lightBase;
	uint lightBaseInt = uint(lightBase) % 64;
	uint lightNextInt = min(lightBaseInt + 1, 63);
	vec2 distortion = sin(mod(time + fragmentData.zw * 5, 3.14159*2)) / 10;
	vec2 texCoords = vec2(fragmentData.z + distortion.y, fragmentData.w + distortion.x);
	vec2 texLevel = textureQueryLod(fragmentTexture, texCoords);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	vec3 fragmentTextureCoords = vec3(texCoords, fragmentTextureIndices.y);
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

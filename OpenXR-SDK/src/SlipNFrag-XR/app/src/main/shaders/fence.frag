#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision mediump int;

layout(push_constant) uniform Lightmap
{
	layout(offset = 0) float lightmapIndex;
};

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 0, binding = 2) uniform usampler2D fragmentColormap;
layout(set = 1, binding = 0) uniform sampler2DArray fragmentLightmap;
layout(set = 2, binding = 0) uniform usampler2D fragmentTexture;

layout(location = 0) in vec4 fragmentData;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec3 lightmapSize = textureSize(fragmentLightmap, 0);
	vec3 lightmapCoords = vec3(fragmentData.xy / lightmapSize.xy, lightmapIndex);
	vec4 lightmapEntry = texture(fragmentLightmap, lightmapCoords);
	uint light = (uint(lightmapEntry.x) / 256) % 64;
	vec2 texLevel = textureQueryLod(fragmentTexture, fragmentData.zw);
	vec2 texMip = vec2(floor(texLevel.y), ceil(texLevel.y));
	uvec4 lowTexEntry = textureLod(fragmentTexture, fragmentData.zw, texMip.x);
	uvec4 highTexEntry = textureLod(fragmentTexture, fragmentData.zw, texMip.y);
	uvec4 lowColormapped = texelFetch(fragmentColormap, ivec2(lowTexEntry.x, light), 0);
	uvec4 highColormapped = texelFetch(fragmentColormap, ivec2(highTexEntry.x, light), 0);
	if (lowColormapped.x == 255 || highColormapped.x == 255)
	{
		discard;
	}
	vec4 lowColor = palette[lowColormapped.x];
	vec4 highColor = palette[highColormapped.x];
	outColor = mix(lowColor, highColor, texLevel.y - texMip.x);
}

#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 1, binding = 0) uniform usampler2DArray fragmentTexture;

layout(push_constant) uniform Turbulent
{
	layout(offset = 0) vec4 tint;
	layout(offset = 16) float time;
};

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 1) in flat int fragmentTextureIndex;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 distortion = sin(mod(time + fragmentTexCoords * 5, 3.14159*2)) / 10;
	vec2 texCoords = vec2(fragmentTexCoords.x + distortion.y, fragmentTexCoords.y + distortion.x);
	vec2 level = textureQueryLod(fragmentTexture, texCoords);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	vec3 fragmentTextureCoords = vec3(texCoords, fragmentTextureIndex);
	vec4 lowColor = textureLod(fragmentTexture, fragmentTextureCoords, mip.x);
	vec4 highColor = textureLod(fragmentTexture, fragmentTextureCoords, mip.y);
	vec4 color = mix(lowColor, highColor, level.y - mip.x) / 256;
	vec4 gammaCorrectedColor = vec4(
		((color.r < 0.04045) ? (color.r / 12.92) : (pow((color.r + 0.055) / 1.055, 2.4))),
		((color.g < 0.04045) ? (color.g / 12.92) : (pow((color.g + 0.055) / 1.055, 2.4))),
		((color.b < 0.04045) ? (color.b / 12.92) : (pow((color.b + 0.055) / 1.055, 2.4))),
		1
	);
	outColor = mix(gammaCorrectedColor, tint, tint.a);
}

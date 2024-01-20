#version 460

precision highp float;
precision highp int;

layout(set = 1, binding = 0) uniform usampler2DArray fragmentTexture;

layout(push_constant) uniform Turbulent
{
	layout(offset = 0) vec4 tint;
	layout(offset = 16) float gamma;
	layout(offset = 20) float time;
};

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 1) in flat int fragmentTextureIndex;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 distortion = sin(mod(time + fragmentTexCoords * 5, 3.14159*2)) / 10;
	vec2 texCoords = fragmentTexCoords.xy + distortion.yx;
	vec2 level = textureQueryLod(fragmentTexture, texCoords);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	vec3 fragmentTextureCoords = vec3(texCoords, fragmentTextureIndex);
	vec4 lowColor = textureLod(fragmentTexture, fragmentTextureCoords, mip.x);
	vec4 highColor = textureLod(fragmentTexture, fragmentTextureCoords, mip.y);
	vec4 color = mix(lowColor, highColor, level.y - mip.x);
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

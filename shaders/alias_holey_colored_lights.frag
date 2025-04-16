#version 460

precision mediump float;
precision mediump int;

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(set = 1, binding = 0) uniform usampler2D fragmentTexture;

layout(push_constant) uniform Tint
{
	layout(offset = 64) vec4 tint;
	layout(offset = 80) float gamma;
};

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 1) in vec4 fragmentLight;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec2 level = textureQueryLod(fragmentTexture, fragmentTexCoords);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	uvec4 lowEntry = textureLod(fragmentTexture, fragmentTexCoords, mip.x);
	uvec4 highEntry = textureLod(fragmentTexture, fragmentTexCoords, mip.y);
	if (lowEntry.x == 255 || highEntry.x == 255)
	{
		discard;
	}
	vec4 lowColor = palette[lowEntry.x];
	vec4 highColor = palette[highEntry.x];
	vec4 color =
		mix(lowColor, highColor, level.y - mip.x) *
		((lowEntry.x >= 224 || highEntry.x >= 224) ? vec4(255, 255, 255, 255) : max(fragmentLight, vec4(255, 255, 255, 255)));
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

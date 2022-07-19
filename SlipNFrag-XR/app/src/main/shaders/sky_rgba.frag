#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 1, binding = 0) uniform usampler2D fragmentTexture;

layout(push_constant) uniform Rotation
{
	layout(offset = 0) float forwardX;
	layout(offset = 4) float forwardY;
	layout(offset = 8) float forwardZ;
	layout(offset = 12) float rightX;
	layout(offset = 16) float rightY;
	layout(offset = 20) float rightZ;
	layout(offset = 24) float upX;
	layout(offset = 28) float upY;
	layout(offset = 32) float upZ;
	layout(offset = 36) float width;
	layout(offset = 40) float height;
	layout(offset = 44) float maxSize;
	layout(offset = 48) float speed;
};

layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	int u = int(fragmentTexCoords.x * int(width));
	int v = int(fragmentTexCoords.y * int(height));
	float	wu, wv, temp;
	vec3	end;
	temp = maxSize;
	wu = 8192.0f * float(u-int(int(width)>>1)) / temp;
	wv = 8192.0f * float(int(int(height)>>1)-v) / temp;
	end[0] = 4096.0f*forwardX + wu*rightX + wv*upX;
	end[1] = 4096.0f*forwardY + wu*rightY + wv*upY;
	end[2] = 4096.0f*forwardZ + wu*rightZ + wv*upZ;
	end[2] *= 3;
	end = normalize(end);
	temp = speed;
	float s = float((temp + 6*(128/2-1)*end[0]));
	float t = float((temp + 6*(128/2-1)*end[1]));
	vec2 texCoords = vec2(s / 128.0f, t / 128.0f);
	vec2 level = textureQueryLod(fragmentTexture, texCoords);
	vec2 mip = vec2(floor(level.y), ceil(level.y));
	vec4 lowColor = textureLod(fragmentTexture, texCoords, 0);
	vec4 highColor = textureLod(fragmentTexture, texCoords, 0);
	vec4 color = mix(lowColor, highColor, level.y - mip.x) / 256;
	vec4 gammaCorrectedColor = vec4(
		((color.r < 0.04045) ? (color.r / 12.92) : (pow((color.r + 0.055) / 1.055, 2.4))),
		((color.g < 0.04045) ? (color.g / 12.92) : (pow((color.g + 0.055) / 1.055, 2.4))),
		((color.b < 0.04045) ? (color.b / 12.92) : (pow((color.b + 0.055) / 1.055, 2.4))),
		1
	);
	outColor = gammaCorrectedColor;
}

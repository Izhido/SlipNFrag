#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 0, binding = 0) uniform sampler2D fragmentPalette;
layout(set = 1, binding = 1) uniform sampler2D fragmentTexture;
layout(set = 1, binding = 2) uniform Rotation
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
	float lowMip = floor(level.y);
	float highMip = ceil(level.y);
	vec4 lowEntry = textureLod(fragmentTexture, texCoords, lowMip);
	vec4 highEntry = textureLod(fragmentTexture, texCoords, highMip);
	vec4 lowColor = texelFetch(fragmentPalette, ivec2(lowEntry.x * 255.0, 0), 0);
	vec4 highColor = texelFetch(fragmentPalette, ivec2(highEntry.x * 255.0, 0), 0);
	float delta = level.y - lowMip;
	float r = lowColor.x + delta * (highColor.x - lowColor.x);
	float g = lowColor.y + delta * (highColor.y - lowColor.y);
	float b = lowColor.z + delta * (highColor.z - lowColor.z);
	float a = lowColor.w + delta * (highColor.w - lowColor.w);
	outColor = vec4(r, g, b, a);
	gl_FragDepth = 1;
}

#version 460

precision mediump float;
precision mediump int;

layout(set = 0, binding = 1) uniform Palette
{
	layout(offset = 0) vec4 palette[256];
};

layout(location = 0) in flat int fragmentColor;

layout(location = 0) out lowp vec4 outColor;

void main()
{
	outColor = palette[fragmentColor];
}

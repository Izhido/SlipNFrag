#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

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

#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision mediump float;
precision mediump int;

layout(set = 0, binding = 1) uniform sampler2D fragmentPalette;

layout(push_constant) uniform Color
{
	layout(offset = 0) float inColor;
};

layout(location = 0) out lowp vec4 outColor;

void main()
{
	outColor = texelFetch(fragmentPalette, ivec2(inColor, 0), 0);
}

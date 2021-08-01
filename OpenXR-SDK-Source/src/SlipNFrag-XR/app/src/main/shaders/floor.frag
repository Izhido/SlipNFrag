#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision mediump float;
precision mediump int;

layout(set = 1, binding = 0) uniform sampler2D fragmentTexture;
layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	outColor = texture(fragmentTexture, fragmentTexCoords);
}

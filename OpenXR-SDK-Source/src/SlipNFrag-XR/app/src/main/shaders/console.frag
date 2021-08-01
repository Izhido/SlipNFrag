#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision mediump float;
precision mediump int;

layout(set = 0, binding = 0) uniform usampler2D fragmentTexture;
layout(set = 0, binding = 1) uniform sampler2D fragmentPalette;
layout(location = 0) in vec2 fragmentTexCoords;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	vec4 entry = texture(fragmentTexture, fragmentTexCoords);
	outColor = texelFetch(fragmentPalette, ivec2(entry.x, 0), 0);
}

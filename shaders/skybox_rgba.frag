#version 450

precision mediump float;
precision mediump int;

layout(set = 1, binding = 0) uniform sampler2DArray fragmentTexture;
layout(location = 0) in vec4 fragmentTexCoords;
layout(location = 0) out lowp vec4 outColor;

void main()
{
	outColor = texture(fragmentTexture, fragmentTexCoords.xyz);
}

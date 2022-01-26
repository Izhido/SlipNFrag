#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_EXT_multiview : enable

precision mediump float;
precision mediump int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in float vertexColor;
layout(location = 0) out int fragmentColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	gl_Position = projectionMatrix[gl_ViewIndex] * (viewMatrix[gl_ViewIndex] * (vertexTransform * vec4(vertexPosition, 1)));
	fragmentColor = int(vertexColor);
}

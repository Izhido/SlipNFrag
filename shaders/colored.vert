#version 460

#extension GL_EXT_multiview : require

precision mediump float;
precision mediump int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in float vertexColor;
layout(location = 0) out int fragmentColor;

void main(void)
{
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * vertexPosition;
	fragmentColor = int(vertexColor);
}

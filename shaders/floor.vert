#version 460

#extension GL_EXT_multiview : require

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
};

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 0) out mediump vec2 fragmentTexCoords;

void main(void)
{
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexPosition;
	fragmentTexCoords = vertexTexCoords;
}

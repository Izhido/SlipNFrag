#version 450

#extension GL_EXT_multiview : require

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
};

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec4 vertexTexCoords;
layout(location = 0) out vec4 fragmentTexCoords;

void main(void)
{
	mat4 view = mat4(mat3(viewMatrix[gl_ViewIndex]));
	gl_Position = (projectionMatrix[gl_ViewIndex] * view * vertexPosition).xyww;
	fragmentTexCoords = vertexTexCoords;
}

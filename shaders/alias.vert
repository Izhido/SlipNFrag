#version 450

#extension GL_EXT_multiview : require

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(set = 0, binding = 2) readonly buffer Lighting
{
	float lightData[];
};

layout(push_constant) uniform Transforms
{
	layout(offset = 0) mat4 aliasTransform;
	layout(offset = 64) int lightOffset;
};

layout(location = 0) in uvec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 0) out vec2 fragmentTexCoords;
layout(location = 1) out float fragmentLight;

void main(void)
{
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * aliasTransform * vec4(vertexPosition, 1);
	fragmentTexCoords = vertexTexCoords;
	fragmentLight = lightData[lightOffset + gl_VertexIndex];
}

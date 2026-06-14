#version 450

#extension GL_EXT_multiview : require

precision highp float;
precision highp int;

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
	layout(offset = 84) int attributeOffset;
};

layout(location = 0) in uvec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 0) out vec2 fragmentTexCoords;
layout(location = 1) out vec4 fragmentLight;

void main(void)
{
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * aliasTransform * vec4(vertexPosition, 1);
	fragmentTexCoords = vertexTexCoords;
	int lightIndex = attributeOffset + gl_VertexIndex * 3;
	fragmentLight = vec4(lightData[lightIndex], lightData[lightIndex + 1], lightData[lightIndex + 2], 255) / vec4(32, 32, 32, 1);
}

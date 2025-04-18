#version 460

#extension GL_EXT_multiview : require

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(push_constant) uniform Transforms
{
	layout(offset = 0) mat4 aliasTransform;
};

layout(location = 0) in uvec4 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 2) in float vertexLight;
layout(location = 0) out vec4 fragmentData;

void main(void)
{
	vec4 position = viewMatrix[gl_ViewIndex] * vertexTransform * aliasTransform * vertexPosition;
	gl_Position = projectionMatrix[gl_ViewIndex] * position;
	fragmentData = vec4(vertexTexCoords, vertexLight, clamp(-position.z * 9 - 2, 0, 1));
}

#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_EXT_multiview : enable

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(push_constant) uniform Transforms
{
	layout(offset = 0) mat4 aliasTransform;
	layout(offset = 64) float forwardX;
	layout(offset = 68) float forwardY;
	layout(offset = 72) float forwardZ;
	layout(offset = 76) float offset;
};

layout(location = 0) in uvec4 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 2) in float vertexLight;
layout(location = 0) out mediump vec4 fragmentData;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	vec4 position = vertexTransform * aliasTransform * vertexPosition;
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * position;
	float projection = offset + position.x * forwardX + position.y * forwardY + position.z * forwardZ;
	fragmentData = vec4(vertexTexCoords, vertexLight, clamp(projection / 8, 0, 1));
}

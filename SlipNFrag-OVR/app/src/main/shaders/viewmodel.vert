#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 ViewMatrix;
	layout(offset = 64) mat4 ProjectionMatrix;
};

layout(push_constant) uniform Transforms
{
	layout(offset = 0) mat4 aliasTransform;
	layout(offset = 64) float forwardX;
	layout(offset = 68) float forwardY;
	layout(offset = 72) float forwardZ;
	layout(offset = 76) float offset;
};

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 2) in float vertexLight;
layout(location = 3) in mat4 vertexTransform;
layout(location = 0) out mediump vec2 fragmentTexCoords;
layout(location = 1) out mediump float fragmentLight;
layout(location = 2) out mediump float fragmentAlpha;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	vec4 position = vertexTransform * (aliasTransform * vertexPosition);
	gl_Position = ProjectionMatrix * (ViewMatrix * position);
	fragmentTexCoords = vertexTexCoords;
	fragmentLight = vertexLight;
	float projection = offset + position.x * forwardX + position.y * forwardY + position.z * forwardZ;
	fragmentAlpha = min(max(projection / 8, 0), 1);
}

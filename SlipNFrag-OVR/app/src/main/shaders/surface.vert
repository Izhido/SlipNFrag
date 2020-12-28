#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 ViewMatrix;
	layout(offset = 64) mat4 ProjectionMatrix;
};

layout(push_constant) uniform Origin
{
	layout(offset = 0) float originX;
	layout(offset = 4) float originY;
	layout(offset = 8) float originZ;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 2) in mat4 vertexTransform;
layout(location = 0) out mediump vec2 fragmentTexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	gl_Position = ProjectionMatrix * (ViewMatrix * (vertexTransform * vec4(vertexPosition.x + originX, vertexPosition.y + originY, vertexPosition.z + originZ, 1)));
	fragmentTexCoords = vertexTexCoords;
}

#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision highp int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 ViewMatrix;
	layout(offset = 64) mat4 ProjectionMatrix;
};

layout(push_constant) uniform Transforms
{
	layout(offset = 0) float originX;
	layout(offset = 4) float originY;
	layout(offset = 8) float originZ;
	layout(offset = 12) float vecs00;
	layout(offset = 16) float vecs01;
	layout(offset = 20) float vecs02;
	layout(offset = 24) float vecs03;
	layout(offset = 28) float vecs10;
	layout(offset = 32) float vecs11;
	layout(offset = 36) float vecs12;
	layout(offset = 40) float vecs13;
	layout(offset = 44) float width;
	layout(offset = 48) float height;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in mat4 vertexTransform;
layout(location = 0) out vec2 fragmentTexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	gl_Position = ProjectionMatrix * (ViewMatrix * (vertexTransform * vec4(vertexPosition.x + originX, vertexPosition.z + originZ, -vertexPosition.y - originY, 1)));
	float s = vertexPosition.x * vecs00 + vertexPosition.y * vecs01 + vertexPosition.z * vecs02 + vecs03;
	float t = vertexPosition.x * vecs10 + vertexPosition.y * vecs11 + vertexPosition.z * vecs12 + vecs13;
	fragmentTexCoords = vec2(s / width, t / height);
}

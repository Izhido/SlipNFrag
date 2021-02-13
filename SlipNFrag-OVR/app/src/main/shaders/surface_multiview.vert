#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_OVR_multiview2 : enable

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 ViewMatrix[2];
	layout(offset = 128) mat4 ProjectionMatrix[2];
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
	layout(offset = 44) float texturemins0;
	layout(offset = 48) float texturemins1;
	layout(offset = 52) float extents0;
	layout(offset = 56) float extents1;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in mat4 vertexTransform;
layout(location = 0) out mediump vec2 fragmentTexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	gl_Position = ProjectionMatrix[gl_ViewID_OVR] * (ViewMatrix[gl_ViewID_OVR] * (vertexTransform * vec4(vertexPosition.x + originX, vertexPosition.z + originZ, -vertexPosition.y - originY, 1)));
	float s = vertexPosition.x * vecs00 + vertexPosition.y * vecs01 + vertexPosition.z * vecs02 + vecs03;
	float t = vertexPosition.x * vecs10 + vertexPosition.y * vecs11 + vertexPosition.z * vecs12 + vecs13;
	fragmentTexCoords = vec2((s - texturemins0) / extents0, (t - texturemins1) / extents1);
}

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
	layout(offset = 12) float yaw;
	layout(offset = 16) float pitch;
	layout(offset = 20) float roll;
	layout(offset = 24) float vecs00;
	layout(offset = 28) float vecs01;
	layout(offset = 32) float vecs02;
	layout(offset = 36) float vecs03;
	layout(offset = 40) float vecs10;
	layout(offset = 44) float vecs11;
	layout(offset = 48) float vecs12;
	layout(offset = 52) float vecs13;
	layout(offset = 56) float width;
	layout(offset = 60) float height;
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
	mat4 translation = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, originX, originZ, -originY, 1);
	float yawRadians = yaw * 3.14159 / 180;
	mat4 yawRotation = mat4(cos(yawRadians), 0, sin(yawRadians), 0, 0, 1, 0, 0, -sin(yawRadians), 0, cos(yawRadians), 0, 0, 0, 0, 1);
	float pitchRadians = pitch * 3.14159 / 180;
	mat4 pitchRotation = mat4(cos(pitchRadians), -sin(pitchRadians), 0, 0, sin(pitchRadians), cos(pitchRadians), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	float rollRadians = -roll * 3.14159 / 180;
	mat4 rollRotation = mat4(1, 0, 0, 0, 0, cos(rollRadians), -sin(rollRadians), 0, 0, sin(rollRadians), cos(rollRadians), 0, 0, 0, 0, 1);
	vec4 position = vec4(vertexPosition.x, vertexPosition.z, -vertexPosition.y, 1);
	gl_Position = ProjectionMatrix * ViewMatrix * vertexTransform * translation * rollRotation * pitchRotation * yawRotation * position;
	float s = vertexPosition.x * vecs00 + vertexPosition.y * vecs01 + vertexPosition.z * vecs02 + vecs03;
	float t = vertexPosition.x * vecs10 + vertexPosition.y * vecs11 + vertexPosition.z * vecs12 + vecs13;
	fragmentTexCoords = vec2(s / width, t / height);
}

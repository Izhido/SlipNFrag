#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

precision highp float;
precision mediump int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 ViewMatrix;
	layout(offset = 64) mat4 ProjectionMatrix;
};

layout(push_constant) uniform Transforms
{
	layout(offset = 0) vec4 vecs0;
	layout(offset = 16) vec4 vecs1;
	layout(offset = 32) vec2 texturemins;
	layout(offset = 40) vec2 extents;
	layout(offset = 48) vec2 textureSize;
	layout(offset = 56) float originX;
	layout(offset = 60) float originY;
	layout(offset = 64) float originZ;
	layout(offset = 68) float yaw;
	layout(offset = 72) float pitch;
	layout(offset = 76) float roll;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in mat4 vertexTransform;
layout(location = 0) out vec2 fragmentLightmapCoords;
layout(location = 1) out vec2 fragmentTexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	vec4 position = vec4(vertexPosition.x, vertexPosition.z, -vertexPosition.y, 1);
	mat4 translation = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, originX, originZ, -originY, 1);
	vec3 sine = vec3(sin(yaw), sin(pitch), sin(roll));
	vec3 cosine = vec3(cos(yaw), cos(pitch), cos(roll));
	mat4 yawRotation = mat4(cosine.x, 0, sine.x, 0, 0, 1, 0, 0, -sine.x, 0, cosine.x, 0, 0, 0, 0, 1);
	mat4 pitchRotation = mat4(cosine.y, -sine.y, 0, 0, sine.y, cosine.y, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	mat4 rollRotation = mat4(1, 0, 0, 0, 0, cosine.z, -sine.z, 0, 0, sine.z, cosine.z, 0, 0, 0, 0, 1);
	gl_Position = ProjectionMatrix * ViewMatrix * vertexTransform * translation * rollRotation * pitchRotation * yawRotation * position;
	vec4 position4 = vec4(vertexPosition, 1);
	vec2 texCoords = vec2(dot(position4, vecs0), dot(position4, vecs1));
	vec2 lightmapSize = vec2((int(extents.x) >> 4) + 1, (int(extents.y) >> 4) + 1);
	fragmentLightmapCoords = (((texCoords - texturemins) * (lightmapSize - 1) / extents) + 0.5) / lightmapSize;
	fragmentTexCoords = texCoords / textureSize;
}

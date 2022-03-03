#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_EXT_multiview : enable

precision highp float;
precision mediump int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(push_constant) uniform Transforms
{
	layout(offset = 4) float originX;
	layout(offset = 8) float originY;
	layout(offset = 12) float originZ;
	layout(offset = 16) float yaw;
	layout(offset = 20) float pitch;
	layout(offset = 24) float roll;
};

layout(location = 0) in vec4 vertex;
layout(location = 1) in mat4 texturePosition;
layout(location = 0) out vec4 fragmentData;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	vec4 position = vec4(vertex.x, vertex.z, -vertex.y, vertex.w);
	mat4 translation = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, originX, originZ, -originY, 1);
	vec3 sine = vec3(sin(-yaw), sin(pitch), sin(roll));
	vec3 cosine = vec3(cos(-yaw), cos(pitch), cos(roll));
	mat4 yawRotation = mat4(cosine.x, 0, sine.x, 0, 0, 1, 0, 0, -sine.x, 0, cosine.x, 0, 0, 0, 0, 1);
	mat4 pitchRotation = mat4(cosine.y, -sine.y, 0, 0, sine.y, cosine.y, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	mat4 rollRotation = mat4(1, 0, 0, 0, 0, cosine.z, -sine.z, 0, 0, sine.z, cosine.z, 0, 0, 0, 0, 1);
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * translation * rollRotation * pitchRotation * yawRotation * position;
	vec2 texCoords = vec2(dot(vertex, texturePosition[0]), dot(vertex, texturePosition[1]));
	vec2 lightmapSizeMinusOne = vec2(int(texturePosition[2].z) >> 4, int(texturePosition[2].w) >> 4);
	vec2 lightmapCoords = (texCoords - texturePosition[2].xy) * lightmapSizeMinusOne / texturePosition[2].zw;
	fragmentData = vec4(lightmapCoords, texCoords / texturePosition[3].xy);
}

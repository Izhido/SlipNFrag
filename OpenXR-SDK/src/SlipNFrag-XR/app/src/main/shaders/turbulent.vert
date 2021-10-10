#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_OVR_multiview2 : enable

precision highp float;
precision highp int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(push_constant) uniform Transforms
{
	layout(offset = 0) float originX;
	layout(offset = 4) float originY;
	layout(offset = 8) float originZ;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in mat4 texturePosition;
layout(location = 0) out vec2 fragmentTexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	vec4 position = vec4(vertexPosition.x + originX, vertexPosition.z + originZ, -vertexPosition.y - originY, 1);
	gl_Position = projectionMatrix[gl_ViewID_OVR] * viewMatrix[gl_ViewID_OVR] * vertexTransform * position;
	vec4 position4 = vec4(vertexPosition, 1);
	vec2 texCoords = vec2(dot(position4, texturePosition[0]), dot(position4, texturePosition[1]));
	fragmentTexCoords = texCoords / texturePosition[2].xy;
}

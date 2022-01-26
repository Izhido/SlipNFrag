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

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in mat4 texturePosition;
layout(location = 0) out vec4 fragmentData;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	vec4 position = vec4(vertexPosition.x, vertexPosition.z, -vertexPosition.y, 1);
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * position;
	vec4 position4 = vec4(vertexPosition, 1);
	vec2 texCoords = vec2(dot(position4, texturePosition[0]), dot(position4, texturePosition[1]));
	vec2 lightmapSizeMinusOne = vec2(int(texturePosition[2].z) >> 4, int(texturePosition[2].w) >> 4);
	vec2 lightmapCoords = (texCoords - texturePosition[2].xy) * lightmapSizeMinusOne / texturePosition[2].zw;
	fragmentData = vec4(lightmapCoords, texCoords / texturePosition[3].xy);
}

#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_EXT_multiview : enable

precision highp float;
precision highp int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(location = 0) in vec4 vertex;
layout(location = 1) in mat4 texturePosition;
layout(location = 0) out vec4 fragmentCoords;
layout(location = 1) out flat int fragmentLightmapIndex;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	vec4 position = vec4(vertex.x, vertex.z, -vertex.y, vertex.w);
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * position;
	vec2 texCoords = vec2(dot(vertex, texturePosition[0]), dot(vertex, texturePosition[1]));
	vec2 lightmapSizeMinusOne = floor(texturePosition[2].zw / 16);
	vec2 lightmapCoords = (texCoords - texturePosition[2].xy) * lightmapSizeMinusOne / texturePosition[2].zw;
	fragmentCoords = vec4(lightmapCoords, texCoords / texturePosition[3].xy);
	fragmentLightmapIndex = int(texturePosition[3].z);
}

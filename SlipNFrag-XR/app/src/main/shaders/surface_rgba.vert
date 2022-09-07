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

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in mat4 texturePosition;
layout(location = 5) in vec4 glowTexturePosition;
layout(location = 0) out vec4 fragmentCoords;
layout(location = 1) out flat ivec4 fragmentTextureIndices;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * vertexPosition;
	vec2 texCoords = vec2(dot(vertexPosition, texturePosition[0]), dot(vertexPosition, texturePosition[1]));
	vec2 lightmapSizeMinusOne = floor(texturePosition[2].zw / 16);
	vec2 lightmapCoords = (texCoords - texturePosition[2].xy) * lightmapSizeMinusOne / texturePosition[2].zw;
	fragmentCoords = vec4(lightmapCoords, texCoords / texturePosition[3].xy);
	fragmentTextureIndices = ivec4(texturePosition[3].zw, glowTexturePosition.x, 0);
}

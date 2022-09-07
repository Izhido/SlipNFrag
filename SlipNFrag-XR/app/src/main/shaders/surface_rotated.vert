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
layout(location = 5) in vec4 origin;
layout(location = 6) in vec4 angles;
layout(location = 0) out vec4 fragmentCoords;
layout(location = 1) out flat ivec2 fragmentTextureIndices;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	mat4 translation = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, origin.x, origin.y, origin.z, origin.w);
	vec3 sine = vec3(sin(-angles.x), sin(-angles.y), sin(-angles.z));
	vec3 cosine = vec3(cos(-angles.x), cos(-angles.y), cos(-angles.z));
	mat4 yawRotation = mat4(cosine.y, 0, sine.y, 0, 0, 1, 0, 0, -sine.y, 0, cosine.y, 0, 0, 0, 0, 1);
	mat4 pitchRotation = mat4(cosine.x, -sine.x, 0, 0, sine.x, cosine.x, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	mat4 rollRotation = mat4(1, 0, 0, 0, 0, cosine.z, -sine.z, 0, 0, sine.z, cosine.z, 0, 0, 0, 0, 1);
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * translation * rollRotation * pitchRotation * yawRotation * vertexPosition;
	vec2 texCoords = vec2(dot(vertexPosition, texturePosition[0]), dot(vertexPosition, texturePosition[1]));
	vec2 lightmapSizeMinusOne = floor(texturePosition[2].zw / 16);
	vec2 lightmapCoords = (texCoords - texturePosition[2].xy) * lightmapSizeMinusOne / texturePosition[2].zw;
	fragmentCoords = vec4(lightmapCoords, texCoords / texturePosition[3].xy);
	fragmentTextureIndices = ivec2(texturePosition[3].zw);
}

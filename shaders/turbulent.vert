#version 460

#extension GL_EXT_multiview : require

precision highp float;
precision highp int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(set = 1, binding = 0) readonly buffer TextureData
{
	vec4 textureData[];
};

layout(location = 0) in vec4 vertexPosition;
layout(location = 0) out vec2 fragmentTexCoords;
layout(location = 1) out flat int fragmentTextureIndex;

void main(void)
{
	vec4 position = vec4(vertexPosition.xyz, 1);
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * position;
	int attributeIndex = int(vertexPosition.w);
	vec4 vecs0 = textureData[attributeIndex];
	vec4 vecs1 = textureData[attributeIndex + 1];
	vec4 sizesAndIndices = textureData[attributeIndex + 2];
	vec2 texCoords = vec2(dot(position, vecs0), dot(position, vecs1));
	fragmentTexCoords = texCoords / sizesAndIndices.xy;
	fragmentTextureIndex = int(sizesAndIndices.z);
}

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

layout(set = 1, binding = 0) buffer TextureData
{
	vec4 textureData[];
};

layout(location = 0) in vec4 vertexPosition;
layout(location = 0) out vec2 fragmentTexCoords;
layout(location = 1) out flat int fragmentTextureIndex;

void main(void)
{
	vec4 position = vec4(vertexPosition.xyz, 1);
	int attributeIndex = int(vertexPosition.w);
	vec4 origin = textureData[attributeIndex];
	vec4 angles = textureData[attributeIndex + 1];
	mat4 translation = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, origin.x, origin.y, origin.z, origin.w);
	vec3 sine = vec3(sin(-angles.x), sin(-angles.y), sin(-angles.z));
	vec3 cosine = vec3(cos(-angles.x), cos(-angles.y), cos(-angles.z));
	mat4 yawRotation = mat4(cosine.y, 0, sine.y, 0, 0, 1, 0, 0, -sine.y, 0, cosine.y, 0, 0, 0, 0, 1);
	mat4 pitchRotation = mat4(cosine.x, -sine.x, 0, 0, sine.x, cosine.x, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	mat4 rollRotation = mat4(1, 0, 0, 0, 0, cosine.z, -sine.z, 0, 0, sine.z, cosine.z, 0, 0, 0, 0, 1);
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * translation * rollRotation * pitchRotation * yawRotation * position;
	vec4 vecs0 = textureData[attributeIndex + 2];
	vec4 vecs1 = textureData[attributeIndex + 3];
	vec4 sizesAndIndices = textureData[attributeIndex + 4];
	vec2 texCoords = vec2(dot(position, vecs0), dot(position, vecs1));
	fragmentTexCoords = texCoords / sizesAndIndices.xy;
	fragmentTextureIndex = int(sizesAndIndices.z);
}

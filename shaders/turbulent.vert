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

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec4 texturePosition0;
layout(location = 2) in vec4 texturePosition1;
layout(location = 3) in vec4 textureCoords;
layout(location = 0) out vec2 fragmentTexCoords;
layout(location = 1) out flat int fragmentTextureIndex;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * vertexPosition;
	vec2 texCoords = vec2(dot(vertexPosition, texturePosition0), dot(vertexPosition, texturePosition1));
	fragmentTexCoords = texCoords / textureCoords.xy;
	fragmentTextureIndex = int(textureCoords.z);
}

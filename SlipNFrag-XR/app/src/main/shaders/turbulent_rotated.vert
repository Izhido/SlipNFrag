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
layout(location = 1) in vec4 origin;
layout(location = 2) in vec4 angles;
layout(location = 3) in vec4 texturePosition0;
layout(location = 4) in vec4 texturePosition1;
layout(location = 5) in vec4 textureCoords;
layout(location = 0) out vec2 fragmentTexCoords;
layout(location = 1) out flat int fragmentTextureIndex;

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
	vec2 texCoords = vec2(dot(vertexPosition, texturePosition0), dot(vertexPosition, texturePosition1));
	fragmentTexCoords = texCoords / textureCoords.xy;
	fragmentTextureIndex = int(textureCoords.z);
}

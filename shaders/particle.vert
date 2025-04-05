#version 460

#extension GL_EXT_multiview : require

precision mediump float;
precision mediump int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(push_constant) uniform Transforms
{
	layout(offset = 0) vec4 vright;
	layout(offset = 16) vec4 vup;
};

const vec2 offsets[6] = vec2[]
(
	vec2(0, 1),
	vec2(1, 1),
	vec2(1, 0),
	vec2(1, 0),
	vec2(0, 0),
	vec2(0, 1)
);

layout(location = 0) in vec4 vertexData;
layout(location = 0) out int fragmentColor;

void main(void)
{
	vec4 position = vec4(vertexData.xyz, 1) + vright * offsets[gl_VertexIndex].x + vup * offsets[gl_VertexIndex].y;
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * position;
	fragmentColor = int(vertexData.w);
}

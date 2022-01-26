#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_EXT_multiview : enable

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
	vec2(-0.5, 0.5),
	vec2(0.5, 0.5),
	vec2(0.5, -0.5),
	vec2(0.5, -0.5),
	vec2(-0.5, -0.5),
	vec2(-0.5, 0.5)
);

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in float vertexColor;
layout(location = 0) out int fragmentColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	vec4 position = vec4(vertexPosition, 1) + vright * offsets[gl_VertexIndex].x + vup * offsets[gl_VertexIndex].y;
	gl_Position = projectionMatrix[gl_ViewIndex] * (viewMatrix[gl_ViewIndex] * (vertexTransform * position));
	fragmentColor = int(vertexColor);
}

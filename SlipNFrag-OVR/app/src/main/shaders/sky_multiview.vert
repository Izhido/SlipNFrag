#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_OVR_multiview2 : enable

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 ViewMatrix[2];
	layout(offset = 128) mat4 ProjectionMatrix[2];
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 0) out mediump vec2 fragmentTexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	gl_Position = ProjectionMatrix[gl_ViewID_OVR] * vec4(vertexPosition, 1);
	fragmentTexCoords = vertexTexCoords;
}

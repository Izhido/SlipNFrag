#version 440 core

#extension GL_EXT_shader_io_blocks : enable
#extension GL_ARB_enhanced_layouts : enable

layout(binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 ViewMatrix;
	layout(offset = 64) mat4 ProjectionMatrix;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 2) in mat4 vertexTransform;
layout(location = 0) out mediump vec2 fragmentTexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main(void)
{
	gl_Position = ProjectionMatrix * (ViewMatrix * (vertexTransform * vec4(vertexPosition, 1.0)));
	fragmentTexCoords = vertexTexCoords;
}

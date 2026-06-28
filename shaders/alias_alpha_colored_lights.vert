#version 450

#extension GL_EXT_multiview : require

precision highp float;
precision highp int;

layout(set = 0, binding = 0) uniform SceneMatrices
{
	layout(offset = 0) mat4 viewMatrix[2];
	layout(offset = 128) mat4 projectionMatrix[2];
	layout(offset = 256) mat4 vertexTransform;
};

layout(set = 0, binding = 2) readonly buffer Attributes
{
	float data[];
};

layout(push_constant) uniform PushConstants
{
	layout(offset = 0) vec4 tint;
	layout(offset = 16) float gamma;
	layout(offset = 20) int attributeBaseIndex;
};

layout(location = 0) in uvec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 0) out vec2 fragmentTexCoords;
layout(location = 1) out vec4 fragmentAttributes;
layout(location = 2) out flat vec4 fragmentTint;
layout(location = 3) out flat float fragmentGamma;

void main(void)
{
    int index = (attributeBaseIndex + gl_InstanceIndex) * 13;
    mat4 aliasTransform = mat4(
        data[index], data[index + 1], data[index + 2], 0,
        data[index + 3], data[index + 4], data[index + 5], 0,
        data[index + 6], data[index + 7], data[index + 8], 0,
        data[index + 9], data[index + 10], data[index + 11], 1
    );
	gl_Position = projectionMatrix[gl_ViewIndex] * viewMatrix[gl_ViewIndex] * vertexTransform * aliasTransform * vec4(vertexPosition, 1);
	fragmentTexCoords = vertexTexCoords;
	int lightOffset = int(data[index + 12]);
	int lightIndex = lightOffset + gl_VertexIndex * 4;
	fragmentAttributes = vec4(data[lightIndex], data[lightIndex + 1], data[lightIndex + 2], data[lightIndex + 3]) / vec4(32, 32, 32, 1);
	fragmentTint = tint;
	fragmentGamma = gamma;
}

#version 450
// #extension GL_ARB_seperate_sharder_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 deformVec[121];
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 color;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	vec3 alteredPosition = position - ubo.deformVec[gl_VertexIndex];

	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(alteredPosition, 1.0);
	fragColor = position;
	fragTexCoord = texCoord;
}
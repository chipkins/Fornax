#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define NUM_LIGHTS 17

struct Light {
	vec4 position;
	vec4 color;
	float radius;
	float quadraticFalloff;
	float linearFalloff;
	float _pad;
};

layout (binding = 0) uniform UBO 
{
	Light lights[NUM_LIGHTS];
	vec4 viewPos;
	mat4 view;
	mat4 model;
} ubo;

layout (binding = 1) uniform sampler2D samplerPosition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

const float AMBIENT_FACTOR = 0.15;

void main() 
{
	// Get G-Buffer values
	vec3 fragPos = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb * 2.0 - 1.0;
	vec4 color = texture(samplerAlbedo, inUV);
	vec4 fragColor = color * AMBIENT_FACTOR;
	
	if (length(fragPos) == 0.0)
	{
		fragColor = color;
	}
	else
	{	
		for(int i = 0; i < NUM_LIGHTS; ++i)
		{
			// Light to fragment
			vec3 lightPos = vec3(ubo.view * ubo.model * vec4(ubo.lights[i].position.xyz, 1.0));
			vec3 L = lightPos - fragPos;
			float dist = length(L);
			L = normalize(L);

			// Viewer to fragment
			vec3 viewPos = vec3(ubo.view * ubo.model * vec4(ubo.viewPos.xyz, 1.0));
			vec3 V = viewPos - fragPos;
			V = normalize(V);

			// Attenuation
			float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);

			// Diffuse part
			vec3 N = normalize(normal);
			float NdotL = max(0.0, dot(N, L));
			vec3 diff = ubo.lights[i].color.rgb * color.rgb * NdotL * atten;

			// Specular part
			vec3 R = reflect(-L, N);
			float NdotR = max(0.0, dot(R, V));
			vec3 spec = ubo.lights[i].color.rgb * pow(NdotR, 16.0) * (atten * 1.5);

			fragColor += vec4(diff + spec, 1.0);
		}
	}
   
	outFragColor = fragColor;	
}
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform UBO 
{
	float radialBlurScale;
	float radialBlurStrength;
	vec2 radialOrigin;
} ubo;

layout (binding = 1) uniform sampler2D texSampler;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main() 
{
	// ivec2 texDim = textureSize(texSampler, 0);
	// vec2 radialSize = vec2(1.0 / texDim.s, 1.0 / texDim.t); 
	
	// vec2 UV = inUV;
 
	// vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	// UV += radialSize * 0.5 - ubo.radialOrigin;
 
	// #define samples 32

	// for (int i = 0; i < samples; i++) 
	// {
	// 	float scale = 1.0 - ubo.radialBlurScale * (float(i) / float(samples-1));
	// 	color += texture(texSampler, UV * scale + ubo.radialOrigin);
	// }
 
	// outColor = (color / samples) * ubo.radialBlurStrength;
	
	outColor = texture(texSampler, inUV);
}
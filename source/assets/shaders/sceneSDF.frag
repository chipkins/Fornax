#version 450

// const int MAX_STEPS = 255;
// const float MIN_DIST = 0.2;
// const float MAX_DIST = 20.0;
// const float EPSILON = 0.0001;

// // Descriptor Bindings ---------------------------

// layout (binding = 0) uniform UBO 
// {
// 	mat4 view;
// 	mat4 proj;
// 	vec2 resolution;
// } camera;

// layout (binding = 1) uniform vec2 resolution;

// layout (location = 0) in vec2 inUV;

// layout (location = 0) out vec4 outColor;

// // ------------------------------------------------

// // SDF Primitives ---------------------------------

// float sdSphere(vec3 pos, float radius)
// {
// 	return length(pos) - radius;
// }

// // ------------------------------------------------

// float scene(vec3 pos)
// {
// 	return sdSphere(pos, shape.radius);
// }

// float sphereTrace(vec3 eye, vec3 dir)
// {
// 	float depth = MIN_DIST;

// 	for (int i = 0; i < MAX_STEPS; ++i)
// 	{
// 		float dist = scene(eye + depth * dir);
// 		if (dist < EPSILON)
// 		{
// 			return depth;
// 		}
// 		depth += dist;
// 		if (depth >= end)
// 		{
// 			return MAX_DIST;
// 		}
// 	}
// 	return MAX_DIST;
// }

void main() 
{
	// mat4 screenToWorld = inverse(camera.proj * camera.view);
	// vec2 pos = (-resolution + 2.0*gl_FragCoord)/resolution.y;
	// float dist = scene(eye,)
}
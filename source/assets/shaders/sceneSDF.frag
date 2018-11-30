#version 450

precision mediump float;

const int MAX_STEPS = 255;
const float MIN_DIST = 0.2;
const float MAX_DIST = 20.0;
const float EPSILON = 0.0001;

const vec3 light1 = vec3(3.0, 3.0, -3.0);

// Descriptor Bindings ---------------------------

layout (binding = 0) uniform UBO 
{
	mat4 view;
	vec3 eye;
	float fov;
	vec2 resolution;
	float dt;
} camera;

//layout (binding = 1) uniform vec2 resolution;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

// ------------------------------------------------

/**
 * Rotation matrix around the X axis.
 */
mat3 rotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    );
}

/**
 * Rotation matrix around the Y axis.
 */
mat3 rotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    );
}

/**
 * Rotation matrix around the Z axis.
 */
mat3 rotateZ(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, -s, 0),
        vec3(s, c, 0),
        vec3(0, 0, 1)
    );
}

// SDF Primitives ---------------------------------

float sdSphere(vec3 pos, float radius)
{
	return length(pos) - radius;
}

float sdCube(vec3 pos, vec3 bounds)
{
	vec3 d = abs(pos) - bounds;
	return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

float sdCylinder(vec3 p, float h, float r)
{
	float radius = length(p.xy) - r;
	float height = abs(p.z) - h/2.0;

	float inDist = min(max(radius, height), 0.0);
	float outDist = length(max(vec2(radius, height), 0.0));

	return inDist + outDist;
}

// ------------------------------------------------

// SDF Function -----------------------------------

float sdUnion(float distA, float distB)
{
	return min(distA, distB);
}

float sdIntersect(float distA, float distB)
{
	return max(distA, distB);
}

float sdDifference(float distA, distB)
{
	return max(distA, -distB);
}

// ------------------------------------------------

float scene(vec3 pos)
{
	// Slowly rotate the whole scene
	pos = rotateY(camera.dt / 2.0) * pos;

	float cylinderRadius = 0.4 + (1.0 - 0.4) * (1.0 + sin(1.7 * camera.dt)) / 2.0;
	float ballOffset = 0.4 + 1.0 + sin(1.7 * camera.dt);
	float ballRadius = 0.3;

	float res = sdCylinder(pos, 2.0, cylinderRadius);
	res = sdUnion(res, sdCylinder(rotateX(radians(90.0)) * pos, 2.0, cylinderRadius);
	res = sdUnion(res, sdCylinder(rotateY(radians(90.0)) * pos, 2.0, cylinderRadius);
	res = sdDifference(res, sdIntersect(sdCube(pos-vec3(0.0,0.0,0.0),vec3(0.4)), sdSphere(pos-vec3(-1.0,0.0,0.0), 0.5)));
	res = sdUnion(res, sdSphere(pos - vec3(ballOffset, 0.0, 0.0)), ballRadius);
	res = sdUnion(res, sdSphere(pos + vec3(ballOffset, 0.0, 0.0)), ballRadius);
	res = sdUnion(res, sdSphere(pos - vec3(0.0, ballOffset, 0.0)), ballRadius);
	res = sdUnion(res, sdSphere(pos + vec3(0.0, ballOffset, 0.0)), ballRadius);
	res = sdUnion(res, sdSphere(pos - vec3(0.0, 0.0, ballOffset)), ballRadius);
	res = sdUnion(res, sdSphere(pos + vec3(0.0, 0.0, ballOffset)), ballRadius);
	return res;
}

float sphereTrace(vec3 eye, vec3 dir)
{
	float depth = MIN_DIST;

	for (int i = 0; i < MAX_STEPS; ++i)
	{
		float dist = scene(eye + depth * dir);
		if (dist < EPSILON)
		{
			return depth;
		}
		depth += dist;
		if (depth >= MAX_DIST)
		{
			return MAX_DIST;
		}
	}
	return MAX_DIST;
}

vec3 rayDirection(float fov, vec2 size, vec2 fragCoord)
{
    vec2 xy = fragCoord - size / 2.0;
    float z = size.y / tan(radians(fov) / 2.0);
    return normalize(vec3(xy, -z));
}

vec3 estimateNormal(vec3 p)
{
    return normalize(vec3(
        scene(vec3(p.x + EPSILON, p.y, p.z)) - scene(vec3(p.x - EPSILON, p.y, p.z)),
        scene(vec3(p.x, p.y + EPSILON, p.z)) - scene(vec3(p.x, p.y - EPSILON, p.z)),
        scene(vec3(p.x, p.y, p.z  + EPSILON)) - scene(vec3(p.x, p.y, p.z - EPSILON))
    ));
}

void main() 
{
	// mat4 screenToWorld = inverse(camera.proj * camera.view);
	// vec3 screenPos = normalize(vec3(gl_FragCoord.xy - camera.resolution/2.0, 100));
	// vec4 worldPos =  screenToWorld * vec4(screenPos, 1.0);
	// vec3 dir = worldPos.xyz;
	vec3 dir = (camera.view * vec4(rayDirection(camera.fov, camera.resolution, gl_FragCoord.xy), 0.0)).xyz;
	float dist = sphereTrace(camera.eye, dir);

	if (dist > MAX_DIST - EPSILON) // Didn't hit an object
	{
		outColor = vec4(vec3(0.0), 1.0);
		return;
	}

	vec3 p = camera.eye + dist * dir; // Closest point on the surface

	// Calculate lighting
	vec4 ambientColor = vec4(vec3(0.02), 1.0);
	vec4 diffuseColor = vec4(0.7, 0.2, 0.2, 1.0);
	vec4 specularColor = vec4(1.0);
	float shininess = 16.0;

	vec3 N = estimateNormal(p);
	vec3 L = normalize(light1 - p);
	vec3 V = normalize(camera.eye - p);
	vec3 H = normalize(L + V);

	float NdotL = clamp(dot(N, L), 0.0, 1.0);
	float NdotH = clamp(dot(N, H), 0.0, 1.0);

	outColor = ambientColor + (diffuseColor * NdotL) + pow(NdotH, shininess);
}
#include "PhysicsBackend.h"

void PhysicsBackend::UpdatePhysics(float dt)
{
	
}

bool PhysicsBackend::TestPointPlane(glm::vec3 pointPos, glm::vec3 planeOrigin, glm::vec3 planeNormal)
{
	glm::vec3 v = pointPos - planeOrigin;
	float d = glm::dot(v, planeNormal);
	return d <= 0;
}

void PhysicsBackend::ResolveCollision(RigidBody* point, glm::vec3 normal)
{
	glm::vec3 rV = -point->velocity;
	float vN = glm::dot(rV, normal);
	if (vN > 0)
		return;
	float j = -vN;
	j *= point->mass;
	glm::vec3 impulse = j * normal;
	point->netImpulse += impulse * 0.1f;
}
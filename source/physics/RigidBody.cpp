#include "RigidBody.h"

RigidBody::RigidBody(glm::vec3 pos, glm::vec3 vel, glm::vec3 acc, float m)
{
	mass = m;
	invMass = m <= 0.0f ? 0.0f : 1.0f / m;

	position = pos;
	velocity = vel;
	acceleration = acc;

	netForce = glm::vec3(0);
	netImpulse = glm::vec3(0);
}

void RigidBody::ApplyForce(float dt)
{
	acceleration = invMass * netForce;

	glm::vec3 vdt = velocity * dt;
	glm::vec3 aT2 = 0.5f * acceleration * powf(dt, 2);
	position += vdt + aT2;

	velocity += acceleration * dt + invMass * netImpulse;

	netForce = netImpulse = glm::vec3(0);
}
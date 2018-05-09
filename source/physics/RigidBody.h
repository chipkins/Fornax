#pragma once

#include "../PrecompiledHeader.h"

class RigidBody
{
public:
	float mass, invMass;
	glm::vec3 position, velocity, acceleration;
	glm::vec3 netForce, netImpulse;

	RigidBody() {};
	RigidBody(glm::vec3 pos, glm::vec3 vel, glm::vec3 acc, float m);

	void ApplyForce(float dt);
};
#pragma once

#include "../PrecompiledHeader.h"
#include "SBLattice.h"

class PhysicsBackend
{
public:
	void UpdatePhysics(float dt);
	bool TestPointPlane(glm::vec3 pointPos, glm::vec3 planeOrigin, glm::vec3 planeNormal);
	void ResolveCollision(RigidBody* point, glm::vec3 normal);

private:
};
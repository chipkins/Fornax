#pragma once

#include "../PrecompiledHeader.h"
#include "../render/Model.h"
#include "RigidBody.h"

class SBLattice
{
public:
	SBLattice();
	SBLattice(const Model& m, float width, float height, int x, int y, float k, float d);
	~SBLattice();

private:
	int dimensionsX, dimensionsY;

	float restHeight, restWidth;

	uint32_t numRigidBodies;
	RigidBody** bodies;
	glm::vec3** deformation;
	
	float coefficient;
	float dampening;
};
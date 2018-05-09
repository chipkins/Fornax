#pragma once

#include "../PrecompiledHeader.h"
#include "../render/Model.h"
#include "RigidBody.h"

class SBLattice
{
public:
	SBLattice();
	SBLattice(Model m, float width, float height, int x, int y, float k, float d);
	~SBLattice();

	Model mesh;
	glm::vec3 deformVecs[121];

	void Update(float dt);
	void SetNetForce(glm::vec3 force) { externalForce = force; }
	RigidBody** GetBodies() { return bodies; }
	uint32_t GetNumBodies() { return numRigidBodies; }

private:
	int dimensionsX, dimensionsY;

	float restHeight, restWidth;

	uint32_t numRigidBodies;
	RigidBody** bodies;
	glm::vec3** deformation;
	
	float coefficient;
	float dampening;

	glm::vec3 externalForce;
};
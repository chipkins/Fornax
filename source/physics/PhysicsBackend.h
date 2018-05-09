#pragma once

#include "../PrecompiledHeader.h"
#include "SBLattice.h"

class PhysicsBackend
{
public:
	void UpdatePhysics(float dt);

private:
	SBLattice softbody;
};
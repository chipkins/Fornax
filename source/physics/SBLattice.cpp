#include "SBLattice.h"
#include <cstdio>
#include <iostream>

SBLattice::SBLattice()
{
	numRigidBodies = 0;
	coefficient = 0;
	dampening = 0;

	dimensionsX = dimensionsY = 0;
	restHeight = restWidth = 0;
}

SBLattice::SBLattice(vk::Model m, float width, float height, int x, int y, float k, float d)
{
	mesh = m;

	dimensionsX = x;
	dimensionsY = y;

	numRigidBodies = x * y;
	coefficient = k;
	dampening = d;

	restHeight = height;
	restWidth = width;

	bodies = new RigidBody*[y];
	for (int i = 0; i < y; ++i)
	{
		bodies[i] = new RigidBody[x];
		for (int j = 0; j < x; ++j)
		{
			bodies[i][j] = RigidBody(m.getVertices()[i*y + j].pos, glm::vec3(0), glm::vec3(0), 1.0f);
		}
	}
}

SBLattice::~SBLattice()
{
	for (int i = 0; i < dimensionsY; ++i)
	{
		delete[] bodies[i];
	}
	delete[] bodies;
}

void SBLattice::Update(float dt)
{
	glm::vec3 displacement;
	glm::vec3 direction;
	float magnitude;

	for (int i = 0; i < dimensionsY; ++i)
	{
		for (int j = 0; j < dimensionsX; ++j)
		{
			if (i > 0)
			{
				displacement = bodies[i-1][j].position - bodies[i][j].position;
				direction = glm::normalize(displacement);
				magnitude = glm::length(displacement);
				bodies[i][j].netForce += coefficient * (magnitude - restHeight) * direction - bodies[i][j].velocity * dampening;
			}
			else 
				bodies[i][j].netForce += externalForce;
			if (i < dimensionsY - 1)
			{
				displacement = bodies[i+1][j].position - bodies[i][j].position;
				direction = glm::normalize(displacement);
				magnitude = glm::length(displacement);
				bodies[i][j].netForce += coefficient * (magnitude - restHeight) * direction - bodies[i][j].velocity * dampening;
			}
			else 
				bodies[i][j].netForce += externalForce;
			if (j > 0)
			{
				displacement = bodies[i][j-1].position - bodies[i][j].position;
				direction = glm::normalize(displacement);
				magnitude = glm::length(displacement);
				bodies[i][j].netForce += coefficient * (magnitude - restWidth) * direction - bodies[i][j].velocity * dampening;
			}
			else 
				bodies[i][j].netForce += externalForce;
			if (j < dimensionsX - 1)
			{
				displacement = bodies[i][j+1].position - bodies[i][j].position;
				direction = glm::normalize(displacement);
				magnitude = glm::length(displacement);
				bodies[i][j].netForce += coefficient * (magnitude - restWidth) * direction - bodies[i][j].velocity * dampening;
			}
			else 
				bodies[i][j].netForce += externalForce;
		}
	}

	for (int i = 0; i < dimensionsY; ++i)
	{
		for (int j = 0; j < dimensionsX; ++j)
		{
			int numVertex = i * dimensionsY + j;
			bodies[i][j].ApplyForce(dt);
			glm::vec3 rigidPos = bodies[i][j].position;
			glm::vec3 meshPos = mesh.getVertices()[numVertex].pos;
			deformVecs[numVertex] = rigidPos - meshPos;
		}
	}
}
#pragma once

#include "../PrecompiledHeader.h"

enum ColliderType
{
	NONE,
	AABB,
	OOBB,
	SPHERE,
	SOFTBODY,
	COUNT
};

struct Collider
{
	ColliderType colliderType;

	glm::vec3 min;
	glm::vec3 max;
};
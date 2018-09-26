#pragma once

#include "render/VkRenderBackend.h"
#include "physics/PhysicsBackend.h"
#include "physics/SBLattice.h"

class FornaxApp
{
public:
	const int WIDTH  = 800;
	const int HEIGHT = 600;

	FornaxApp();
	~FornaxApp();

	void Run();
	void UpdateAndDraw();

//private:
	GLFWwindow* m_window = nullptr;
	VkRenderBackend* m_renderer;
	PhysicsBackend   m_physics;
	

	Camera m_camera;

	SBLattice* m_softbody;
	struct {
		glm::vec3 origin;
		glm::vec3 normal;
	} m_plane;

	float frameTime;
	float prevFrameTime = 0;
	float startTime;
	float dt;
	const float physicsStep = 0.12;
	float moverate = 0.01f;

	void Cleanup();
	void CreateWindow();
};
#pragma once

#include "render/VkRenderBackend.h"
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

	Camera m_camera;

	SBLattice* m_softbody;

	float frameTime;
	float prevFrameTime;
	float startTime;
	float dt;
	const float physicsStep = 0.12;

	void Cleanup();
	void CreateWindow();
};
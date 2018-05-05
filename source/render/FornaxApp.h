#pragma once

#include "VkRenderBackend.h"

class FornaxApp
{
public:
	const int WIDTH  = 800;
	const int HEIGHT = 600;

	FornaxApp();
	~FornaxApp();

	void Run();

//private:
	GLFWwindow* m_window = nullptr;
	VkRenderBackend* m_renderer;

	Camera m_camera;

	void Cleanup();
	void CreateWindow();
};
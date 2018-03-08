#pragma once

#include "VkRenderBackend.h"

class FornaxApp
{
public:
	const int WIDTH  = 800;
	const int HEIGHT = 600;

	void Run();
	void Cleanup();

	void CreateWindow();

private:
	GLFWwindow* window;
	VkRenderBackend* renderer;
};
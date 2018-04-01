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

private:
	GLFWwindow* window = nullptr;
	VkRenderBackend* renderer;

	void Cleanup();
	void CreateWindow();
};
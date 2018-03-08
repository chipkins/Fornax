#include "FornaxApp.h"

void FornaxApp::Run()
{
	renderer->RequestFrameRender();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

void FornaxApp::Cleanup()
{
	renderer->Cleanup();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void FornaxApp::CreateWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	renderer->Init(window);
}
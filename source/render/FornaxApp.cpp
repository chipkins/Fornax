#include "FornaxApp.h"

static void onWindowResized(GLFWwindow* window, int width, int height)
{
	VkRenderBackend* renderer = reinterpret_cast<VkRenderBackend*>(glfwGetWindowUserPointer(window));
	renderer->RecreateSwapchain();
}

FornaxApp::FornaxApp()
{
	renderer = new VkRenderBackend();
}

FornaxApp::~FornaxApp()
{
	delete renderer;
}

void FornaxApp::Run()
{
	if (window == nullptr)
	{
		CreateWindow();
	}

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		renderer->UpdateUniformBuffer();
		renderer->RequestFrameRender();
	}

	renderer->WaitForDrawFinish();

	Cleanup();
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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(window, renderer);
	glfwSetWindowSizeCallback(window, onWindowResized);

	renderer->Init(window);
}
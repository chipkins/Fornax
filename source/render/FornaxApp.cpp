#include "FornaxApp.h"

double mouseX, mouseY;
double prevMouseX, prevMouseY;

static void onWindowResized(GLFWwindow* window, int width, int height)
{
	auto* app = reinterpret_cast<FornaxApp*>(glfwGetWindowUserPointer(window));
	app->m_renderer->RecreateSwapchain();
	app->m_camera.ResizeCamera(width, height);
}

static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto* app = reinterpret_cast<FornaxApp*>(glfwGetWindowUserPointer(window));

	float moverate = 0.25f;

	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		app->m_camera.Move(glm::vec3(0, 0, 1), moverate);
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		app->m_camera.Move(glm::vec3(0, 0, -1), moverate);
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
		app->m_camera.Move(glm::vec3(1, 0, 0), moverate);
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		app->m_camera.Move(glm::vec3(-1, 0, 0), moverate);
}

static void onMouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	auto* app = reinterpret_cast<FornaxApp*>(glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_LEFT)
		app->m_camera.MouseRotate((mouseY - prevMouseY)*0.00125, (mouseX - prevMouseY)*0.00125);

	glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
}

FornaxApp::FornaxApp()
{
	if (m_window == nullptr)
	{
		CreateWindow();
	}

	m_renderer = new VkRenderBackend();

	int width, height;
	glfwGetWindowSize(m_window, &width, &height);
	m_camera = Camera(width, height);
}

FornaxApp::~FornaxApp()
{
	delete m_renderer;
}

void FornaxApp::Run()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
		m_camera.Update();
		m_renderer->UpdateUniformBuffer(m_camera);
		m_renderer->RequestFrameRender();
	}

	m_renderer->WaitForDrawFinish();

	Cleanup();
}

void FornaxApp::Cleanup()
{
	m_renderer->Cleanup();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void FornaxApp::CreateWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, onWindowResized);

	glfwSetMouseButtonCallback(m_window, onMouseCallback);
	glfwSetKeyCallback(m_window, onKeyCallback);

	m_renderer->Init(m_window);
}
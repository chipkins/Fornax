#include "FornaxApp.h"

#include <chrono>

double mouseX, mouseY;
double prevMouseX, prevMouseY;
bool l_buttonHeld, r_buttonHeld;
glm::vec3 externalForce = glm::vec3(0);

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
		app->m_camera.Move(glm::vec3(0, 0, -1), moverate);
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		app->m_camera.Move(glm::vec3(0, 0, 1), moverate);
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
		app->m_camera.Move(glm::vec3(1, 0, 0), moverate);
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		app->m_camera.Move(glm::vec3(-1, 0, 0), moverate);
}

static void onMouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	auto* app = reinterpret_cast<FornaxApp*>(glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
			l_buttonHeld = true;
		if (action == GLFW_RELEASE)
			l_buttonHeld = false;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
			r_buttonHeld = true;
		if (action == GLFW_RELEASE)
			r_buttonHeld = false;
	}
}

FornaxApp::FornaxApp()
{
	m_renderer = new VkRenderBackend();
	
	if (m_window == nullptr)
	{
		CreateWindow();
	}

	int width, height;
	glfwGetWindowSize(m_window, &width, &height);
	m_camera = Camera(width, height);
	Model model = m_renderer->GetModelList()[0];
	m_softbody = new SBLattice(model, 0.01, 0.01, 100, 100, 100.0f, 0.75f);
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

		glfwGetCursorPos(m_window, &mouseX, &mouseY);
		if (l_buttonHeld)
			m_softbody->SetNetForce(glm::vec3(mouseX - prevMouseX, mouseY - prevMouseY, 0));
		if (r_buttonHeld)
			m_camera.MouseRotate((mouseY - prevMouseY)*0.00125, (mouseX - prevMouseX)*0.00125);
		glfwGetCursorPos(m_window, &prevMouseX, &prevMouseY);

		UpdateAndDraw();
	}

	m_renderer->WaitForDrawFinish();

	Cleanup();
}

void FornaxApp::UpdateAndDraw()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	frameTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	dt = frameTime - prevFrameTime;

	m_camera.Update();
	m_softbody->Update(dt);
	m_renderer->UpdateUniformBuffer(m_camera, frameTime);
	m_renderer->RequestFrameRender();

	prevFrameTime = frameTime;
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
#include "FornaxApp.h"

#include <chrono>
#include <iostream>
#include <unordered_map>

double mouseX, mouseY;
double prevMouseX, prevMouseY;
std::unordered_map<int32_t, bool> heldKeys;
bool l_buttonHeld, r_buttonHeld;
glm::vec3 externalForce = glm::vec3(0);

static void onWindowResized(GLFWwindow* window, int width, int height)
{
	auto* app = reinterpret_cast<FornaxApp*>(glfwGetWindowUserPointer(window));
	app->m_renderer->ResizeWindow();
	app->m_camera.ResizeCamera(width, height);
}

static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto* app = reinterpret_cast<FornaxApp*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_W)
	{
		if (action == GLFW_PRESS)
			heldKeys[key] = true;
		else if (action == GLFW_RELEASE)
			heldKeys[key] = false;
	}

	if (key == GLFW_KEY_S)
	{
		if (action == GLFW_PRESS)
			heldKeys[key] = true;
		else if (action == GLFW_RELEASE)
			heldKeys[key] = false;
	}

	if (key == GLFW_KEY_D)
	{
		if (action == GLFW_PRESS)
			heldKeys[key] = true;
		else if (action == GLFW_RELEASE)
			heldKeys[key] = false;
	}

	if (key == GLFW_KEY_A)
	{
		if (action == GLFW_PRESS)
			heldKeys[key] = true;
		else if (action == GLFW_RELEASE)
			heldKeys[key] = false;
	}

	if (key == GLFW_KEY_LEFT_CONTROL)
	{
		if (action == GLFW_PRESS)
			heldKeys[key] = true;
		else if (action == GLFW_RELEASE)
			heldKeys[key] = false;
	}

	if (key == GLFW_KEY_LEFT_ALT)
	{
		if (action == GLFW_PRESS)
			heldKeys[key] = true;
		else if (action == GLFW_RELEASE)
			heldKeys[key] = false;
	}

	if (key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, 1);
}

static void onMouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	auto* app = reinterpret_cast<FornaxApp*>(glfwGetWindowUserPointer(window));

	if (button == GLFW_MOUSE_BUTTON_1)
	{
		if (action == GLFW_PRESS)
			r_buttonHeld = true;
		else if (action == GLFW_RELEASE)
			l_buttonHeld = false;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
			r_buttonHeld = true;
		else if (action == GLFW_RELEASE)
			r_buttonHeld = false;
	}
}

static void onErrorCallback(int error, const char* description)
{
	puts(description);
}

FornaxApp::FornaxApp()
{
	std::vector<const char*> enabledExtensions;
	m_renderer = new VkRenderBackend(enabledExtensions);
	
	if (m_window == nullptr)
	{
		CreateWindow();
	}

	int width, height;
	glfwGetWindowSize(m_window, &width, &height);
	m_camera = Camera(width, height);
	vk::Model model = m_renderer->GetModelList()[0];
	m_softbody = new SBLattice(model, 0.25f, 0.25f, 11, 11, 25.0f, 0.75f);
	m_plane.origin = glm::vec3(0,  0.5f, 0);
	m_plane.normal = glm::vec3(0, -1.0f, 0);

	std::cout << "\nPress -W- to move the camera forward along the z axis" << std::endl;
	std::cout << "Press -S- to move the camera backward along the z axis" << std::endl;
	std::cout << "Press -A- to move the camera left along the x axis" << std::endl;
	std::cout << "Press -D- to move the camera right along the x axis" << std::endl;
	std::cout << "Press -Left Ctrl- to move the camera up along the y axis" << std::endl;
	std::cout << "Press -Left Alt- to move the camera down along the y axis" << std::endl;
	std::cout << "\nDrag with the -Right Mouse Button- to look around with the camera" << std::endl;
	std::cout << "Drag with the -Left Mouse Button- to apply a force to the SoftBody" << std::endl;
	std:: cout << "\nPress -Esc- to quit the application" << std::endl;
}

FornaxApp::~FornaxApp()
{
	delete m_renderer;
	getchar();
}

void FornaxApp::Run()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		glfwGetCursorPos(m_window, &mouseX, &mouseY);
		if (l_buttonHeld)
			m_softbody->SetNetForce(glm::vec3((prevMouseX - mouseX)*0.25, (mouseY - prevMouseY)*0.25, 0));
		if (r_buttonHeld)
			m_camera.MouseRotate((mouseX - prevMouseX)*0.00125, (mouseY - prevMouseY)*0.00125);
		
		if (heldKeys[GLFW_KEY_W])
			m_camera.Move(glm::vec3(0, 0, 1.0f), moverate);
		if (heldKeys[GLFW_KEY_S])
			m_camera.Move(glm::vec3(0, 0, -1.0f), moverate);
		if (heldKeys[GLFW_KEY_D])
			m_camera.Move(glm::vec3(1.0f, 0, 0), moverate);
		if (heldKeys[GLFW_KEY_A])
			m_camera.Move(glm::vec3(-1.0f, 0, 0), moverate);
		if (heldKeys[GLFW_KEY_LEFT_CONTROL])
			m_camera.MoveYAxis(-moverate);
		if (heldKeys[GLFW_KEY_LEFT_ALT])
			m_camera.MoveYAxis(moverate);

		glfwGetCursorPos(m_window, &prevMouseX, &prevMouseY);

		UpdateAndDraw();
	}

	m_renderer->WaitForDrawToFinish();

	Cleanup();
}

void FornaxApp::UpdateAndDraw()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	frameTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	dt = frameTime - prevFrameTime;

	for (size_t i = 0; i < 11; ++i)
	{
		for (size_t j = 0; j < 11; ++j)
		{
			RigidBody* point = &m_softbody->GetBodies()[i][j];
			if (m_physics.TestPointPlane(point->position, m_plane.origin, m_plane.normal))
				m_physics.ResolveCollision(point, m_plane.normal);
		}
	}

	m_camera.Update();
	m_softbody->Update(dt);
	m_renderer->UpdateUniformBuffers(m_camera, m_softbody->deformVecs, frameTime);
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
	glfwSetErrorCallback(onErrorCallback);

	if (!glfwInit()) 
	{ 
		throw std::runtime_error("failed to initialize glfw");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, onWindowResized);

	glfwSetMouseButtonCallback(m_window, onMouseCallback);
	glfwSetKeyCallback(m_window, onKeyCallback);

	m_renderer->Init(m_window);
}
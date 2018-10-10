#pragma once

#include "VulkanHeader.h"

namespace vk
{
	struct Window
	{
		GLFWwindow*  windowPtr;
		VkSurfaceKHR surface;
		int32_t width, height;

		Window(GLFWwindow* windowPtr)
		{
			this->windowPtr = windowPtr;
		}

		~Window()
		{

		}

		VkResult InitSurface(VkInstance instance)
		{
			return glfwCreateWindowSurface(instance, windowPtr, nullptr, &surface);
		}
	};
}
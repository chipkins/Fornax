#pragma once

#include "vulkan/vulkan.h"
//#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

namespace vk
{
	struct Window
	{
		GLFWwindow*  windowPtr;
		VkSurfaceKHR surface;
		int32_t width, height;

		Window(GLFWwindow* window)
		{
			windowPtr = window;
			glfwGetWindowSize(windowPtr, &width, &height);
		}

		VkResult InitSurface(VkInstance instance)
		{
			return glfwCreateWindowSurface(instance, windowPtr, nullptr, &surface);
		}

		void DestroySurface(VkInstance instance)
		{
			vkDestroySurfaceKHR(instance, surface, nullptr);
		}
	};
}
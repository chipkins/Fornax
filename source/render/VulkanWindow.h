#pragma once

#include "VulkanHeader.h"

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
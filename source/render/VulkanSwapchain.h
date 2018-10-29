#pragma once

#include "VulkanHeader.h"
#include "VulkanWindow.h"

namespace vk
{
	class Swapchain
	{
	private:
		VkInstance       instance;
		VkDevice         logicalDevice;
		VkPhysicalDevice physicalDevice;

	public:
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;

		uint32_t                 imageCount;
		std::vector<VkImage>     images;
		std::vector<VkImageView> imageViews;

		VkFormat                             colorFormat;
		VkColorSpaceKHR                      colorSpace;
		VkExtent2D                           extent;
		VkSurfaceCapabilitiesKHR             surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR>      surfaceFormats;
		std::vector<VkPresentModeKHR>        presentModes;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;

		uint32_t graphicsFamilyIndex = std::numeric_limits<uint32_t>::max();
		uint32_t presentFamilyIndex  = std::numeric_limits<uint32_t>::max();

		void Create(Window* glfwWindow, uint32_t graphicsFamilyIndex, uint32_t presentFamilyIndex, bool vsync = false)
		{
			// Set respective queue family indices
			this->graphicsFamilyIndex = graphicsFamilyIndex;
			this->presentFamilyIndex = presentFamilyIndex;

			VkResult err;
			VkSwapchainKHR oldSwapchain = swapchain;

			// Get physical device surface properties and formats
			err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, glfwWindow->surface, &surfaceCapabilities);
			if (err != VK_SUCCESS)
			{
				throw std::runtime_error("Unable to retrieve surface capabilities.");
			}

			// Get available present modes
			uint32_t presentModeCount;
			err = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, glfwWindow->surface, &presentModeCount, nullptr);
			if (err != VK_SUCCESS || presentModeCount < 1)
			{
				throw std::runtime_error("Cannot obtain surface present modes");
			}
			presentModes.resize(presentModeCount);
			err = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, glfwWindow->surface, &presentModeCount, presentModes.data());
			if (err != VK_SUCCESS)
			{
				throw std::runtime_error("Cannot obtain surface present modes");
			}

			// Choose best suitable presentation mode
			VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
			for (const auto& availablePresentMode : presentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR)
				{
					presentMode = availablePresentMode;
					break;
				}
				else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					presentMode = availablePresentMode;
				}
			}

			// Select Swapchain Extent
			if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				extent = surfaceCapabilities.currentExtent;
			}
			else
			{
				int32_t width, height;
				glfwGetWindowSize(glfwWindow->windowPtr, &width, &height);

				VkExtent2D actualExtent = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};

				extent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, actualExtent.width));
				extent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, actualExtent.height));
			}

			// Determine number of images
			uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
			if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
			{
				imageCount = surfaceCapabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = glfwWindow->surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = colorFormat;
			createInfo.imageColorSpace = colorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			uint32_t queueFamilyIndices[] = { graphicsFamilyIndex, presentFamilyIndex };
			if (graphicsFamilyIndex != presentFamilyIndex)
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0;
				createInfo.pQueueFamilyIndices = nullptr;
			}

			createInfo.preTransform = surfaceCapabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = oldSwapchain;

			if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create swap chain!");
			}

			if (oldSwapchain != VK_NULL_HANDLE)
			{
				for (auto& view : imageViews)
				{
					vkDestroyImageView(logicalDevice, view, nullptr);
				}
				vkDestroySwapchainKHR(logicalDevice, oldSwapchain, nullptr);
			}

			vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
			images.resize(imageCount);
			vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, images.data());

			imageViews.resize(imageCount);
			for (uint32_t i = 0; i < imageCount; ++i)
			{
				VkImageViewCreateInfo viewInfo = {};
				viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewInfo.image = images[i];
				viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewInfo.format = colorFormat;
				viewInfo.components = {
					VK_COMPONENT_SWIZZLE_R,
					VK_COMPONENT_SWIZZLE_G,
					VK_COMPONENT_SWIZZLE_B,
					VK_COMPONENT_SWIZZLE_A
				};
				viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewInfo.subresourceRange.baseMipLevel = 0;
				viewInfo.subresourceRange.levelCount = 1;
				viewInfo.subresourceRange.baseArrayLayer = 0;
				viewInfo.subresourceRange.layerCount = 1;

				if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
				{
					throw std::runtime_error("failed to create image view!");
				}
			}
		}

		void ConnectGLFWSurface(VkSurfaceKHR surface)
		{
			// Create GLFW window surface
			VkResult err;

			// Find suitable surface format
			uint32_t formatCount;
			err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
			if (err != VK_SUCCESS || formatCount < 1)
			{
				throw std::runtime_error("Cannot retrieve GLFW Surface Formats");
			}
			surfaceFormats.resize(formatCount);
			err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());
			if (err != VK_SUCCESS)
			{
				throw std::runtime_error("Cannot retrieve GLFW Surface Formats");
			}
			// Only available format has no prefered format
			if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
				colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			}
			for (const auto& availableFormat : surfaceFormats)
			{
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					colorFormat = availableFormat.format;
					colorSpace = availableFormat.colorSpace;
					break;
				}
			}
		}

		void ConnectVulkan(VkInstance instance, VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
		{
			this->instance = instance;
			this->logicalDevice = logicalDevice;
			this->physicalDevice = physicalDevice;
		}

		VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex)
		{
			return vkAcquireNextImageKHR(logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(), presentCompleteSemaphore, VK_NULL_HANDLE, imageIndex);
		}

		VkResult QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE)
		{
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchain;
			presentInfo.pImageIndices = &imageIndex;
			if (waitSemaphore != VK_NULL_HANDLE)
			{
				presentInfo.pWaitSemaphores = &waitSemaphore;
				presentInfo.waitSemaphoreCount = 1;
			}
			return vkQueuePresentKHR(queue, &presentInfo);
		}

		void Cleanup()
		{
			if (swapchain != VK_NULL_HANDLE)
			{
				for (auto& view : imageViews)
				{
					vkDestroyImageView(logicalDevice, view, nullptr);
				}
				vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
			}
			swapchain = VK_NULL_HANDLE;
		}
	};
}
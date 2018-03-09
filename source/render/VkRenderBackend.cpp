#include "VkRenderBackend.h"

#include <iostream>
#include <cstring>
#include <limits>

#pragma region Static Functions

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT pCallback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, pCallback, pAllocator);
	}
}

#pragma endregion

#pragma region VkRenderBackend Definition

void VkRenderBackend::Init(GLFWwindow* window)
{
	CreateInstance();
	SetupDebugCallback();
	CreateSurface(window);
	SelectPhysicalDevice();
	CreateLogicalDeviceAndQueues();
	CreateSwapChain();
	CreateImageViews();
}

void VkRenderBackend::Cleanup()
{
	for (auto imageView : m_swapchainImageViews)
	{
		vkDestroyImageView(m_context.device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_context.device, m_swapchain, nullptr);
	vkDestroyDevice(m_context.device, nullptr);
	DestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void VkRenderBackend::RequestFrameRender()
{
	
}

void VkRenderBackend::CreateInstance()
{
	if (c_enableValidationLayers && !CheckValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "IMGE 590 Game Physics";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Fornax";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto extensions = GetRequiredExtensions();

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (c_enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(c_validationLayers.size());
		createInfo.ppEnabledLayerNames = c_validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void VkRenderBackend::CreateSurface(GLFWwindow* window)
{
	if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a window!");
	}
}

void VkRenderBackend::SelectPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (IsDeviceSuitable(device))
		{
			m_context.gpu.physicalDevice = device;
			break;
		}
	}

	if (m_context.gpu.physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void VkRenderBackend::CreateLogicalDeviceAndQueues()
{
	FindQueueFamilies(m_context.gpu.physicalDevice);

	float queuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfoVec;

	VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.queueFamilyIndex = m_context.graphicsFamilyIndex;
	graphicsQueueCreateInfo.queueCount = 1;
	graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
	queueCreateInfoVec.push_back(graphicsQueueCreateInfo);

	if (m_context.graphicsFamilyIndex != m_context.presentFamilyIndex)
	{
		VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
		presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		presentQueueCreateInfo.queueFamilyIndex = m_context.presentFamilyIndex;
		presentQueueCreateInfo.queueCount = 1;
		presentQueueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfoVec.push_back(presentQueueCreateInfo);
	}

	m_context.gpu.deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfoVec.size());
	createInfo.pQueueCreateInfos = queueCreateInfoVec.data();
	createInfo.pEnabledFeatures = &m_context.gpu.deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(c_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = c_deviceExtensions.data();

	if (c_enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(c_validationLayers.size());
		createInfo.ppEnabledLayerNames = c_validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_context.gpu.physicalDevice, &createInfo, nullptr, &m_context.device) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device");
	}

	vkGetDeviceQueue(m_context.device, m_context.graphicsFamilyIndex, 0, &m_context.graphicsQueue);
	vkGetDeviceQueue(m_context.device, m_context.presentFamilyIndex, 0, &m_context.presentQueue);
}

void VkRenderBackend::CreateSwapChain()
{
	//QuerySwapChainSupport(m_context.gpu.physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(m_context.gpu.surfaceFormats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(m_context.gpu.presentModes);
	VkExtent2D extent = ChooseSwapExtent(m_context.gpu.surfaceCapabilities);

	uint32_t imageCount = m_context.gpu.surfaceCapabilities.minImageCount + 1;
	if (m_context.gpu.surfaceCapabilities.maxImageCount > 0 && imageCount > m_context.gpu.surfaceCapabilities.maxImageCount)
	{
		imageCount = m_context.gpu.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = {(uint32_t)m_context.graphicsFamilyIndex, (uint32_t)m_context.presentFamilyIndex};

	if (m_context.graphicsFamilyIndex != m_context.presentFamilyIndex)
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

	createInfo.preTransform = m_context.gpu.surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_context.device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	m_swapchainImageFormat = surfaceFormat.format;
	m_swapchainExtent = extent;

	vkGetSwapchainImagesKHR(m_context.device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_context.device, m_swapchain, &imageCount, m_swapchainImages.data());
}

void VkRenderBackend::CreateImageViews()
{
	m_swapchainImageViews.resize(m_swapchainImages.size());

	for (size_t i = 0; i < m_swapchainImages.size(); ++i)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapchainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_context.device, &createInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void VkRenderBackend::CreateGraphicsPipeline()
{
	
}

void VkRenderBackend::SetupDebugCallback()
{
	if (!c_enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = DebugCallback;

	if (CreateDebugReportCallbackEXT(m_instance, &createInfo, nullptr, &m_callback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug callback");
	}
}

bool VkRenderBackend::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : c_validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char*> VkRenderBackend::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (c_enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

bool VkRenderBackend::IsDeviceSuitable(VkPhysicalDevice device)
{
	vkGetPhysicalDeviceProperties(device, &m_context.gpu.deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &m_context.gpu.deviceFeatures);

	bool isDiscrete = m_context.gpu.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

	FindQueueFamilies(device);
	bool queueFamiliesFound = m_context.graphicsFamilyIndex > -1 && m_context.presentFamilyIndex > -1;

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		QuerySwapChainSupport(device);
		swapChainAdequate = !m_context.gpu.surfaceFormats.empty() && !m_context.gpu.presentModes.empty();
	}

	return isDiscrete && queueFamiliesFound && extensionsSupported && swapChainAdequate;
}

bool VkRenderBackend::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	m_context.gpu.extensionProperties.resize(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, m_context.gpu.extensionProperties.data());

	for (const char* extensionName : c_deviceExtensions)
	{
		bool extensionFound = false;

		for (const auto& extension : m_context.gpu.extensionProperties)
		{
			
			if (strcmp(extensionName, extension.extensionName) == 0)
			{
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound)
		{
			return false;
		}
	}

	return true;
}

void VkRenderBackend::FindQueueFamilies(VkPhysicalDevice device)
{
	m_context.graphicsFamilyIndex = -1;
	m_context.presentFamilyIndex = -1;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	m_context.gpu.queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, m_context.gpu.queueFamilyProperties.data());

	int32_t i = 0;
	for (const auto& queueFamily : m_context.gpu.queueFamilyProperties)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_context.graphicsFamilyIndex = i;
		}

		if (queueFamily.queueCount > 0 && presentSupport)
		{
			m_context.presentFamilyIndex = i;
		}

		if (m_context.graphicsFamilyIndex > -1 && m_context.presentFamilyIndex > -1)
		{
			break;
		}

		++i;
	}
}

void VkRenderBackend::QuerySwapChainSupport(VkPhysicalDevice device)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &m_context.gpu.surfaceCapabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		m_context.gpu.surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, m_context.gpu.surfaceFormats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
	if (presentModeCount)
	{
		m_context.gpu.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, m_context.gpu.presentModes.data());
	}
}

VkSurfaceFormatKHR VkRenderBackend::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VkRenderBackend::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR)
		{
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D VkRenderBackend::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = {800, 600};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));

		return actualExtent;
	}
}

#pragma endregion
#include "VkRenderBase.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <chrono>

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
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
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
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, pCallback, pAllocator);
	}
}

static std::vector<char> ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkRenderBase::VkRenderBase(std::vector<const char*> enabledExtensions)
{
	m_deviceExtensions.insert(m_deviceExtensions.end(), enabledExtensions.begin(), enabledExtensions.end());
}
 
void VkRenderBase::Init(GLFWwindow* window)
{
	CreateInstance();
	SetupDebugCallback();
	CreateGLFWSurface(window);
	SelectPhysicalDevice();
	CreateLogicalDeviceAndQueues();
	SetupSwapchain();
	SetupRenderPass();
	CreatePipelineCache();
	CreateCommandPool();
	SetupDepthStencil();
	SetupFrameBuffers();
	CreateCommandBuffers();
	CreateSemaphores();

	// Preset submition info for the graphics queue
	m_submitInfo = vk::initializers::SubmitInfo();
	m_submitInfo.pWaitDstStageMask = &m_submitPipelineStages;
	m_submitInfo.waitSemaphoreCount = 1;
	m_submitInfo.pWaitSemaphores = &m_semaphores.imageAvailable;
	m_submitInfo.signalSemaphoreCount = 1;
	m_submitInfo.pSignalSemaphores = &m_semaphores.renderFinished;
}

void VkRenderBase::Cleanup()
{
	m_swapchain.Cleanup();

	vkDestroyImageView(m_device->logicalDevice, m_depthStencil.view, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_depthStencil.image, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_depthStencil.memory, nullptr);

	vkDestroyRenderPass(m_device->logicalDevice, m_context.renderPass, nullptr);

	for (auto framebuffer : m_framebuffers)
	{
		vkDestroyFramebuffer(m_device->logicalDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_device->logicalDevice, m_commandPool, 1, m_commandBuffers.data());

	vkDestroySemaphore(m_device->logicalDevice, m_semaphores.imageAvailable, nullptr);
	vkDestroySemaphore(m_device->logicalDevice, m_semaphores.renderFinished, nullptr);
	
	vkDestroyPipelineCache(m_device->logicalDevice, m_pipelineCache, nullptr);

	if (m_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_device->logicalDevice, m_descriptorPool, nullptr);
	}
	vkDestroyCommandPool(m_device->logicalDevice, m_commandPool, nullptr);
	
	vkDestroyDevice(m_device->logicalDevice, nullptr);
	if (c_enableValidationLayers)
	{
		DestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
	}

	delete m_device;
	m_window->DestroySurface(m_instance);
	delete m_window;

	if (c_enableValidationLayers)
	{
		DestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
	}

	vkDestroyInstance(m_instance, nullptr);
}

VkResult VkRenderBase::CreateInstance()
{
	VkResult err;

	if (c_enableValidationLayers && !CheckValidationLayerSupport())
	{
		return VK_ERROR_VALIDATION_FAILED_EXT;
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Fornax Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Fornax";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto requiredExtensions = GetRequiredExtensions();
	m_deviceExtensions.insert(m_deviceExtensions.end(), requiredExtensions.begin(), requiredExtensions.end());

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

	if (c_enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	err = vkCreateInstance(&createInfo, nullptr, &m_instance);

	return err;
}

void VkRenderBase::CreateGLFWSurface(GLFWwindow* window)
{
	m_window = new vk::Window(window);
	m_window->InitSurface(m_instance);
}

void VkRenderBase::SelectPhysicalDevice()
{
	VkPhysicalDevice physicalDevice;

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	m_device = new vk::Device(devices, m_window->surface, m_deviceExtensions);

	if (m_device->physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void VkRenderBase::SetupSwapchain()
{
	m_swapchain.ConnectVulkan(m_instance, m_device->logicalDevice, m_device->physicalDevice);
	m_swapchain.ConnectGLFWSurface(m_window->surface);
	m_swapchain.Create(m_window, m_device->queueFamilyIndices.graphics, m_device->queueFamilyIndices.present);
}

void VkRenderBase::CreateLogicalDeviceAndQueues()
{
	VkPhysicalDeviceFeatures enabledFeatures = {};

	if (m_device->CreateLogicalDevice(enabledFeatures, m_deviceExtensions, m_validationLayers) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device");
	}

	vkGetDeviceQueue(m_device->logicalDevice, m_device->queueFamilyIndices.graphics, 0, &m_context.graphicsQueue);
	vkGetDeviceQueue(m_device->logicalDevice, m_device->queueFamilyIndices.graphics, 0, &m_context.presentQueue);
}

void VkRenderBase::SetupRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_swapchain.colorFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_device->logicalDevice, &renderPassInfo, nullptr, &m_context.renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void VkRenderBase::SetupDepthStencil()
{
	m_context.depthFormat = FindDepthFormat();

	m_device->CreateImage(m_swapchain.extent.width, m_swapchain.extent.height, m_context.depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthStencil.image, m_depthStencil.memory);

	m_depthStencil.view = m_device->CreateImageView(m_depthStencil.image, m_context.depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	m_device->TransitionImageLayout(m_depthStencil.image, m_context.depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_context.graphicsQueue);
}

void VkRenderBase::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_device->queueFamilyIndices.graphics;
	poolInfo.flags = 0;

	if (vkCreateCommandPool(m_device->logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool");
	}
}

void VkRenderBase::CreateCommandBuffers()
{
	m_commandBuffers.resize(m_swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	if (vkAllocateCommandBuffers(m_device->logicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void VkRenderBase::CreatePipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	if (vkCreatePipelineCache(m_device->logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));
}

VkPipelineShaderStageCreateInfo VkRenderBase::LoadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	auto shaderCode = ReadFile(fileName);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_device->logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = stage;
	shaderStageCreateInfo.module = shaderModule;
	shaderStageCreateInfo.pName = "main";

	return shaderStageCreateInfo;
}

void VkRenderBase::SetupFrameBuffers()
{
	m_swapchainFramebuffers.resize(m_swapchain.imageViews.size());

	for (size_t i = 0; i < m_swapchain.imageViews.size(); ++i)
	{
		std::array<VkImageView, 2> attachments = {
			m_swapchain.imageViews[i],
			m_depthStencil.view
		};

		VkFramebufferCreateInfo framebufferInfo = vk::initializers::FramebufferCreateInfo();
		framebufferInfo.renderPass = m_context.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_swapchain.extent.width;
		framebufferInfo.height = m_swapchain.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_device->logicalDevice, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VkRenderBase::CreateSemaphores()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = vk::initializers::SemaphoreCreateInfo();
	if (vkCreateSemaphore(m_device->logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphores.imageAvailable) != VK_SUCCESS ||
		vkCreateSemaphore(m_device->logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphores.renderFinished) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores");
	}
}

void VkRenderBase::SetupDebugCallback()
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

void VkRenderBase::ResizeWindow()
{
	glfwGetWindowSize(m_window->windowPtr, &m_window->width, &m_window->height);
	if (m_window->width == 0 || m_window->height == 0) return;

	WaitForDrawToFinish();

	CleanupSwapchain();

	SetupSwapchain();
	SetupRenderPass();
	SetupDepthStencil();
	SetupFrameBuffers();
	CreateCommandBuffers();
}

void VkRenderBase::CleanupSwapchain()
{
	vkDestroyImageView(m_device->logicalDevice, m_depthStencil.view, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_depthStencil.image, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_depthStencil.memory, nullptr);

	for (auto framebuffer : m_swapchainFramebuffers)
	{
		vkDestroyFramebuffer(m_device->logicalDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_device->logicalDevice, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());

	vkDestroyRenderPass(m_device->logicalDevice, m_context.renderPass, nullptr);

	m_swapchain.Cleanup();
}

bool VkRenderBase::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_validationLayers)
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

std::vector<const char*> VkRenderBase::GetRequiredExtensions()
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

VkFormat VkRenderBase::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_device->physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat VkRenderBase::FindDepthFormat()
{
	return FindSupportedFormat(
	{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void VkRenderBase::WaitForDrawToFinish()
{
	vkDeviceWaitIdle(m_device->logicalDevice);
}

void VkRenderBase::PrepareFrame()
{
	m_swapchain.AcquireNextImage(m_semaphores.imageAvailable, &m_currentBuffer);
}

void VkRenderBase::SubmitFrame()
{
	m_swapchain.QueuePresent(m_context.graphicsQueue, m_currentBuffer, m_semaphores.renderFinished);

	WaitForDrawToFinish();
}
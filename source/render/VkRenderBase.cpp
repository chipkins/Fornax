#include "VkRenderBase.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <limits>
#include <algorithm>
#include <chrono>
 
void VkRenderBase::Init(GLFWwindow* window)
{
	m_window = window;

	//m_models.emplace_back();
	//m_models[0].LoadModel("../source/assets/models/quad.obj");
	//m_models.emplace_back();
	//m_models[1].LoadModel("../source/assets/models/quad.obj");

	CreateInstance();
	SelectPhysicalDevice();
	CreateLogicalDeviceAndQueues();
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPool();
	CreateDepthResources();
	CreateFrameBuffers();
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateCommandBuffers();
	CreateSephamores();
}

void VkRenderBase::Cleanup()
{
	CleanupSwapchain();

	vkDestroyImageView(m_context.device, m_offScreenFrameBuffer.color.view, nullptr);
	vkDestroyImage(m_context.device, m_offScreenFrameBuffer.color.image, nullptr);
	vkFreeMemory(m_context.device, m_offScreenFrameBuffer.color.memory, nullptr);

	vkDestroyImageView(m_context.device, m_offScreenFrameBuffer.depth.view, nullptr);
	vkDestroyImage(m_context.device, m_offScreenFrameBuffer.depth.image, nullptr);
	vkFreeMemory(m_context.device, m_offScreenFrameBuffer.depth.memory, nullptr);

	vkDestroyRenderPass(m_context.device, m_offScreenFrameBuffer.renderPass, nullptr);
	vkDestroySampler(m_context.device, m_offScreenFrameBuffer.sampler, nullptr);
	vkDestroyFramebuffer(m_context.device, m_offScreenFrameBuffer.frameBuffer, nullptr);

	vkDestroyPipeline(m_context.device, m_pipelines.blur, nullptr);
	vkDestroyPipeline(m_context.device, m_pipelines.scene, nullptr);

	vkDestroyPipelineLayout(m_context.device, m_pipelineLayouts.blur, nullptr);
	vkDestroyPipelineLayout(m_context.device, m_pipelineLayouts.scene, nullptr);

	vkDestroyDescriptorSetLayout(m_context.device, m_descriptorSetLayouts.blur, nullptr);
	vkDestroyDescriptorSetLayout(m_context.device, m_descriptorSetLayouts.scene, nullptr);

	m_uniformBuffers.blur.destroy();
	m_uniformBuffers.scene.destroy();

	vkFreeCommandBuffers(m_context.device, m_commandPool, 1, &m_offScreenFrameBuffer.commandBuffer);
	vkDestroySemaphore(m_context.device, m_offScreenFrameBuffer.semaphore, nullptr);

	/*vkDestroySemaphore(m_context.device, m_imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_context.device, m_renderFinishedSemaphore, nullptr);
	vkDestroyDescriptorPool(m_context.device, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_context.device, m_descriptorSetLayout, nullptr);
	vkDestroySampler(m_context.device, m_textureSampler, nullptr);
	vkDestroyImageView(m_context.device, m_textureImageView, nullptr);
	vkDestroyImage(m_context.device, m_textureImage, nullptr);
	vkFreeMemory(m_context.device, m_textureImageMemory, nullptr);
	vkDestroyBuffer(m_context.device, m_uniformBuffer, nullptr);
	vkFreeMemory(m_context.device, m_uniformBufferMemory, nullptr);
	vkDestroyBuffer(m_context.device, m_vertexBuffer, nullptr);
	vkFreeMemory(m_context.device, m_vertexBufferMemory, nullptr);
	vkDestroyBuffer(m_context.device, m_indexBuffer, nullptr);
	vkFreeMemory(m_context.device, m_indexBufferMemory, nullptr);
	vkDestroyCommandPool(m_context.device, m_commandPool, nullptr);*/
	vkDestroyDevice(m_context.device, nullptr);
	if (c_enableValidationLayers)
	{
		DestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
	}
	vkDestroySurfaceKHR(m_instance, m_window.surface, nullptr);
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

	auto extensions = GetRequiredExtensions();

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

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

VkPhysicalDevice VkRenderBase::SelectPhysicalDevice()
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

	for (const auto& device : devices)
	{
		if (IsDeviceSuitable(device))
		{
			physicalDevice = device;
			break;
		}
	}

	if (m_context.device.physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	return physicalDevice;
}

void VkRenderBase::CreateSwapchain()
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
	createInfo.surface = m_window.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { (uint32_t)m_context.graphicsFamilyIndex, (uint32_t)m_context.presentFamilyIndex };

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

void VkRenderBase::SetupRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_swapchainImageFormat;
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

	if (vkCreateRenderPass(m_context.device, &renderPassInfo, nullptr, &m_context.renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void VkRenderBase::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_context.graphicsFamilyIndex;
	poolInfo.flags = 0;

	if (vkCreateCommandPool(m_context.device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
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

	if (vkAllocateCommandBuffers(m_context.device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < m_commandBuffers.size(); ++i)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_context.renderPass;
		renderPassInfo.framebuffer = m_swapchainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapchainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
		uint32_t indexCount = 0;
		for (auto& model : m_models)
		{
			vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(model.getNumIndices()), 1, indexCount, 0, 0);

			indexCount += model.getNumIndices();
		}

		vkCmdEndRenderPass(m_commandBuffers[i]);

		if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void VkRenderBase::CreatePipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	if (vkCreatePipelineCache(m_context.device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));
}

void VkRenderBase::SetupFrameBuffers()
{
	m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

	for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
	{
		std::array<VkImageView, 2> attachments = {
			m_swapchainImageViews[i],
			m_offScreenFrameBuffer.depth.view
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_context.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_swapchainExtent.width;
		framebufferInfo.height = m_swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_context.device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
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

void VkRenderBase::WaitForDrawToFinish()
{
	vkDeviceWaitIdle(m_context.device);
}
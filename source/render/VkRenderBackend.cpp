#include "VkRenderBackend.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <limits>
#include <algorithm>
#include <chrono>

#pragma region Static Functions

#pragma endregion

void VkRenderBackend::Init(GLFWwindow* window)
{
	VkRenderBase::Init(window);

	SetupDescriptorPool();

	m_resources.pipelineLayouts = new vk::PipelineLayoutList(m_device->logicalDevice);
	m_resources.pipelines = new vk::PipelineList(m_device->logicalDevice);
	m_resources.descriptorSetLayouts = new vk::DescriptorSetLayoutList(m_device->logicalDevice);
	m_resources.descriptorSets = new vk::DescriptorSetList(m_device->logicalDevice, m_descriptorPool);

	/*CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();*/

	PrepareOffscreenFramebuffers();
	PrepareUniformBuffers();
	SetupLayoutsAndDescriptors();
	PreparePipelines();
	BuildCommandBuffers();
	BuildDeferredCommandBuffer();

	SetupLights();
}

void VkRenderBackend::Cleanup()
{
	delete m_resources.pipelineLayouts;
	delete m_resources.pipelines;
	delete m_resources.descriptorSetLayouts;
	delete m_resources.descriptorSets;

	m_uniformBuffers.blur.destroy();
	m_uniformBuffers.scene.destroy();
	m_uniformBuffers.lights.destroy();

	/*vkDestroyImageView(m_device->logicalDevice, m_textureImageView, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_textureImage, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_textureImageMemory, nullptr);
	vkDestroySampler(m_device->logicalDevice, m_textureSampler, nullptr);*/

	// Cleanup offscreen framebuffer
	// Color attachments
	for (auto& attachment : m_framebuffers.offscreen.attachments)
	{
		vkDestroyImageView(m_device->logicalDevice, attachment.view, nullptr);
		vkDestroyImage(m_device->logicalDevice, attachment.image, nullptr);
		vkFreeMemory(m_device->logicalDevice, attachment.memory, nullptr);
	}

	vkDestroyImageView(m_device->logicalDevice, m_framebuffers.lights.attachments[0].view, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_framebuffers.lights.attachments[0].image, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_framebuffers.lights.attachments[0].memory, nullptr);

	// Depth attachment
	vkDestroyImageView(m_device->logicalDevice, m_framebuffers.offscreen.depth.view, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_framebuffers.offscreen.depth.image, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_framebuffers.offscreen.depth.memory, nullptr);

	vkDestroyImageView(m_device->logicalDevice, m_framebuffers.lights.depth.view, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_framebuffers.lights.depth.image, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_framebuffers.lights.depth.memory, nullptr);

	vkFreeCommandBuffers(m_device->logicalDevice, m_commandPool, 1, &m_offscreenCommandBuffer);
	vkDestroySemaphore(m_device->logicalDevice, m_offscreenSemaphore, nullptr);

	// Cleanup offscreen framebuffer
	vkDestroyRenderPass(m_device->logicalDevice, m_framebuffers.offscreen.renderPass, nullptr);
	vkDestroyFramebuffer(m_device->logicalDevice, m_framebuffers.offscreen.frameBuffer, nullptr);
	//vkDestroySampler(m_device->logicalDevice, m_framebuffers.offscreen.sampler, nullptr);

	// Cleanup Post-Proccess blur framebuffer
	vkDestroyRenderPass(m_device->logicalDevice, m_framebuffers.lights.renderPass, nullptr);
	vkDestroyFramebuffer(m_device->logicalDevice, m_framebuffers.lights.frameBuffer, nullptr);
	//vkDestroySampler(m_device->logicalDevice, m_framebuffers.blur.sampler, nullptr);

	VkRenderBase::Cleanup();
}

void VkRenderBackend::CreateAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment, uint32_t width, uint32_t height)
{
	VkImageAspectFlags aspectMask = 0;
	VkImageLayout imageLayout;

	attachment->format = format;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	assert(aspectMask > 0);

	m_device->CreateImage(512, 512, format, VK_IMAGE_TILING_OPTIMAL, usage | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment->image, attachment->memory);

	attachment->view = m_device->CreateImageView(attachment->image, format, aspectMask);
}

// Setup the offscreen framebuffer for rendering the blurred scene
// The color attachment of this framebuffer will then be used to sample frame in the fragment shader of the final pass
void VkRenderBackend::PrepareOffscreenFramebuffers()
{
	m_framebuffers.offscreen.width = m_framebuffers.lights.width = 512;
	m_framebuffers.offscreen.height = m_framebuffers.lights.height = 512;
	
	// Color attachment
	CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &m_framebuffers.offscreen.attachments[0], 512, 512);
	CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &m_framebuffers.offscreen.attachments[1], 512, 512);
	CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &m_framebuffers.offscreen.attachments[2], 512, 512);

	// Find a suitable depth format
	VkFormat depthFormat = FindDepthFormat();

	CreateAttachment(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &m_framebuffers.offscreen.depth, 512, 512);

	// Create a separate render pass for the offscreen rendering G-Buffer
	{
		std::array<VkAttachmentDescription, 4> attchmentDescriptions = {};

		for (uint32_t i = 0; i < static_cast<uint32_t>(attchmentDescriptions.size()); ++i)
		{
			// Color attachment
			attchmentDescriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attchmentDescriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attchmentDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attchmentDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attchmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attchmentDescriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attchmentDescriptions[i].finalLayout = (i == 3) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		attchmentDescriptions[0].format = m_framebuffers.offscreen.attachments[0].format;
		attchmentDescriptions[1].format = m_framebuffers.offscreen.attachments[1].format;
		attchmentDescriptions[2].format = m_framebuffers.offscreen.attachments[2].format;
		attchmentDescriptions[3].format = m_framebuffers.offscreen.depth.format;

		std::vector<VkAttachmentReference> colorReferences;
		colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		VkAttachmentReference depthReference = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
		subpassDescription.pColorAttachments = colorReferences.data();
		subpassDescription.pDepthStencilAttachment = &depthReference;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
		renderPassInfo.pAttachments = attchmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		vkCreateRenderPass(m_device->logicalDevice, &renderPassInfo, nullptr, &m_framebuffers.offscreen.renderPass);

		std::array<VkImageView, 4> attachments;
		attachments[0] = m_framebuffers.offscreen.attachments[0].view;
		attachments[1] = m_framebuffers.offscreen.attachments[1].view;
		attachments[2] = m_framebuffers.offscreen.attachments[2].view;
		attachments[3] = m_framebuffers.offscreen.depth.view;

		VkFramebufferCreateInfo fbufCreateInfo = vk::initializers::FramebufferCreateInfo();
		fbufCreateInfo.renderPass = m_framebuffers.offscreen.renderPass;
		fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbufCreateInfo.pAttachments = attachments.data();
		fbufCreateInfo.width = m_framebuffers.offscreen.width;
		fbufCreateInfo.height = m_framebuffers.offscreen.height;
		fbufCreateInfo.layers = 1;

		vkCreateFramebuffer(m_device->logicalDevice, &fbufCreateInfo, nullptr, &m_framebuffers.offscreen.frameBuffer);
	}

	CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &m_framebuffers.lights.attachments[0], 512, 512);

	// Create a separate render pass for the offscreen rendering Post-Proccess Blur
	{
		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.format = m_framebuffers.lights.attachments[0].format;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = &colorReference;
		subpass.colorAttachmentCount = 1;

		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pAttachments = &attachmentDescription;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();
		
		vkCreateRenderPass(m_device->logicalDevice, &renderPassInfo, nullptr, &m_framebuffers.lights.renderPass);

		VkFramebufferCreateInfo fbufCreateInfo = vk::initializers::FramebufferCreateInfo();
		fbufCreateInfo.renderPass = m_framebuffers.lights.renderPass;
		fbufCreateInfo.pAttachments = &m_framebuffers.lights.attachments[0].view;
		fbufCreateInfo.attachmentCount = 1;
		fbufCreateInfo.width = m_framebuffers.lights.width;
		fbufCreateInfo.height = m_framebuffers.lights.height;
		fbufCreateInfo.layers = 1;
		
		vkCreateFramebuffer(m_device->logicalDevice, &fbufCreateInfo, nullptr, &m_framebuffers.lights.frameBuffer);
	}

	// Create sampler to sample from the attachment in the fragment shader
	VkSamplerCreateInfo samplerInfo = vk::initializers::SamplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_colorSampler);
}

// Sets up the command buffer that renders the scene to the offscreen frame buffer
void VkRenderBackend::BuildDeferredCommandBuffer()
{
	if (m_offscreenCommandBuffer == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vk::initializers::CommandBufferAllocateInfo(
				m_commandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		vkAllocateCommandBuffers(m_device->logicalDevice, &cmdBufAllocateInfo, &m_offscreenCommandBuffer);
	}
	if (m_offscreenSemaphore == VK_NULL_HANDLE)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = vk::initializers::SemaphoreCreateInfo();
		vkCreateSemaphore(m_device->logicalDevice, &semaphoreCreateInfo, nullptr, &m_offscreenSemaphore);
	}

	VkCommandBufferBeginInfo cmdBufInfo = vk::initializers::CommandBufferBeginInfo();

	// First Pass : G-Buffer Scene
	std::array<VkClearValue, 4> clearValues = {};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[3].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vk::initializers::RenderPassBeginInfo();
	renderPassBeginInfo.renderPass = m_framebuffers.offscreen.renderPass;
	renderPassBeginInfo.framebuffer = m_framebuffers.offscreen.frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = m_framebuffers.offscreen.width;
	renderPassBeginInfo.renderArea.extent.height = m_framebuffers.offscreen.height;
	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkBeginCommandBuffer(m_offscreenCommandBuffer, &cmdBufInfo);

	VkViewport viewport = vk::initializers::Viewport((float)m_framebuffers.offscreen.width, (float)m_framebuffers.offscreen.height, 0.0f, 1.0f);
	vkCmdSetViewport(m_offscreenCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor = vk::initializers::Rect2D(m_framebuffers.offscreen.width, m_framebuffers.offscreen.height, 0, 0);
	vkCmdSetScissor(m_offscreenCommandBuffer, 0, 1, &scissor);

	vkCmdBeginRenderPass(m_offscreenCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindDescriptorSets(m_offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipelineLayouts->get("scene"), 0, 1, m_resources.descriptorSets->getPtr("scene"), 0, NULL);
	vkCmdBindPipeline(m_offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipelines->get("scene"));

	vkCmdDraw(m_offscreenCommandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(m_offscreenCommandBuffer);

	// Second Pass : Post-Proccess Blur
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassBeginInfo.framebuffer = m_framebuffers.lights.frameBuffer;
	renderPassBeginInfo.renderPass = m_framebuffers.lights.renderPass;
	renderPassBeginInfo.renderArea.extent.width = m_framebuffers.lights.width;
	renderPassBeginInfo.renderArea.extent.height = m_framebuffers.lights.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_offscreenCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	viewport = vk::initializers::Viewport((float)m_framebuffers.lights.width, (float)m_framebuffers.lights.height, 0.0f, 1.0f);
	vkCmdSetViewport(m_offscreenCommandBuffer, 0, 1, &viewport);
	scissor = vk::initializers::Rect2D(m_framebuffers.lights.width, m_framebuffers.lights.height, 0, 0);
	vkCmdSetScissor(m_offscreenCommandBuffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(m_offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipelineLayouts->get("lights"), 0, 1, m_resources.descriptorSets->getPtr("lights"), 0, NULL);
	vkCmdBindPipeline(m_offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipelines->get("lights"));
	vkCmdDraw(m_offscreenCommandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(m_offscreenCommandBuffer);

	vkEndCommandBuffer(m_offscreenCommandBuffer);
}

void VkRenderBackend::BuildCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBufInfo = vk::initializers::CommandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = defaultClearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vk::initializers::RenderPassBeginInfo();
	renderPassBeginInfo.renderPass = m_context.renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_window->width;
	renderPassBeginInfo.renderArea.extent.height = m_window->height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < m_commandBuffers.size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = m_swapchainFramebuffers[i];

		vkBeginCommandBuffer(m_commandBuffers[i], &cmdBufInfo);

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vk::initializers::Viewport((float)m_window->width, (float)m_window->height, 0.0f, 1.0f);
		vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

		VkRect2D scissor = vk::initializers::Rect2D(m_window->width, m_window->height, 0, 0);
		vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };

		// 3D scene
		/*vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipelineLayouts->get("scene"), 0, 1, m_resources.descriptorSets->getPtr("scene"), 0, NULL);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipelines->get("scene"));*/

		// Fullscreen triangle (clipped to a quad) with radial blur
		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipelineLayouts->get("blur"), 0, 1, m_resources.descriptorSets->getPtr("blur"), 0, NULL);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipelines->get("blur"));
		vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_commandBuffers[i]);

		vkEndCommandBuffer(m_commandBuffers[i]);
	}
}

void VkRenderBackend::SetupDescriptorPool()
{
	// Example uses three ubos and one image sampler
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		vk::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
		vk::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10)
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo =
		vk::initializers::DescriptorPoolCreateInfo(
			poolSizes.size(),
			poolSizes.data(),
			4);

	vkCreateDescriptorPool(m_device->logicalDevice, &descriptorPoolInfo, nullptr, &m_descriptorPool);
}

void VkRenderBackend::SetupLayoutsAndDescriptors()
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
	VkDescriptorSetLayoutCreateInfo descriptorSetLayout;
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo;
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	std::vector<VkDescriptorImageInfo> imageDescriptors;
	VkDescriptorSet targetDescriptorSet;

	// G-Buffer creation (offscreen scene rendering)
	setLayoutBindings = {
		vk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),				// Vertex shader uniform buffer
	};
	descriptorSetLayout = vk::initializers::DescriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
	m_resources.descriptorSetLayouts->add("scene", descriptorSetLayout);
	pipelineLayoutCreateInfo = vk::initializers::PipelineLayoutCreateInfo(m_resources.descriptorSetLayouts->getPtr("scene"), 1);
	m_resources.pipelineLayouts->add("scene", pipelineLayoutCreateInfo);
	descriptorSetAllocInfo = vk::initializers::DescriptorSetAllocateInfo(m_descriptorPool, m_resources.descriptorSetLayouts->getPtr("scene"), 1);
	targetDescriptorSet = m_resources.descriptorSets->add("scene", descriptorSetAllocInfo);
	writeDescriptorSets = {
		vk::initializers::WriteDescriptorSet(targetDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &m_uniformBuffers.scene.descriptor),// Binding 0 : Vertex shader uniform buffer			
	};
	vkUpdateDescriptorSets(m_device->logicalDevice, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

	// Deferred lights
	setLayoutBindings =
	{
		vk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),				// Fragment shader uniform buffer
		vk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),		// Position texture target / Scene colormap
		vk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),		// Normals texture target
		vk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),		// Albedo texture target
	};
	descriptorSetLayout = vk::initializers::DescriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
	m_resources.descriptorSetLayouts->add("lights", descriptorSetLayout);
	pipelineLayoutCreateInfo = vk::initializers::PipelineLayoutCreateInfo(m_resources.descriptorSetLayouts->getPtr("lights"), 1);
	m_resources.pipelineLayouts->add("lights", pipelineLayoutCreateInfo);
	descriptorSetAllocInfo = vk::initializers::DescriptorSetAllocateInfo(m_descriptorPool, m_resources.descriptorSetLayouts->getPtr("lights"), 1);
	targetDescriptorSet = m_resources.descriptorSets->add("lights", descriptorSetAllocInfo);
	imageDescriptors = {
		vk::initializers::DescriptorImageInfo(m_colorSampler, m_framebuffers.offscreen.attachments[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
		vk::initializers::DescriptorImageInfo(m_colorSampler, m_framebuffers.offscreen.attachments[1].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
		vk::initializers::DescriptorImageInfo(m_colorSampler, m_framebuffers.offscreen.attachments[2].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
		//vk::initializers::DescriptorImageInfo(m_colorSampler, m_framebuffers.blur.attachments[0].view, VK_IMAGE_LAYOUT_GENERAL),
	};
	writeDescriptorSets = {
		vk::initializers::WriteDescriptorSet(targetDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &m_uniformBuffers.lights.descriptor),		// Binding 1 : Fragment shader uniform buffer
		vk::initializers::WriteDescriptorSet(targetDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &imageDescriptors[0]),				// Binding 2 : Position texture target			
		vk::initializers::WriteDescriptorSet(targetDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &imageDescriptors[1]),				// Binding 3 : Normals texture target			
		vk::initializers::WriteDescriptorSet(targetDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &imageDescriptors[2])				// Binding 4 : Albedo texture target			
	};
	vkUpdateDescriptorSets(m_device->logicalDevice, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

	// Fullscreen radial blur
	setLayoutBindings =
	{
		vk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	VK_SHADER_STAGE_FRAGMENT_BIT, 0),			// Binding 0 : Fragment shader uniform buffer
		vk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	VK_SHADER_STAGE_FRAGMENT_BIT, 1)	// Binding 1 : Fragment shader image sampler
	};
	descriptorSetLayout = vk::initializers::DescriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
	m_resources.descriptorSetLayouts->add("blur", descriptorSetLayout);
	pipelineLayoutCreateInfo = vk::initializers::PipelineLayoutCreateInfo(m_resources.descriptorSetLayouts->getPtr("blur"), 1);
	m_resources.pipelineLayouts->add("blur", pipelineLayoutCreateInfo);
	descriptorSetAllocInfo = vk::initializers::DescriptorSetAllocateInfo(m_descriptorPool, m_resources.descriptorSetLayouts->getPtr("blur"), 1);
	targetDescriptorSet = m_resources.descriptorSets->add("blur", descriptorSetAllocInfo);
	imageDescriptors = {
		vk::initializers::DescriptorImageInfo(m_colorSampler, m_framebuffers.lights.attachments[0].view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
	};
	writeDescriptorSets =
	{
		vk::initializers::WriteDescriptorSet(targetDescriptorSet,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &m_uniformBuffers.blur.descriptor),			// Binding 0: Vertex shader uniform buffer
		vk::initializers::WriteDescriptorSet(targetDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &imageDescriptors[0])	// Binding 1 : Fragment shader texture sampler
	};

	vkUpdateDescriptorSets(m_device->logicalDevice, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
}

void VkRenderBackend::PreparePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
		vk::initializers::PipelineInputAssemblyStateCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			0,
			VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterizationState =
		vk::initializers::PipelineRasterizationStateCreateInfo(
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
			VK_FRONT_FACE_CLOCKWISE,
			0);

	VkPipelineColorBlendAttachmentState blendAttachmentState =
		vk::initializers::PipelineColorBlendAttachmentState(
			0xf,
			VK_FALSE);

	VkPipelineColorBlendStateCreateInfo colorBlendState =
		vk::initializers::PipelineColorBlendStateCreateInfo(
			1,
			&blendAttachmentState);

	VkPipelineDepthStencilStateCreateInfo depthStencilState =
		vk::initializers::PipelineDepthStencilStateCreateInfo(
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS_OR_EQUAL);

	VkPipelineViewportStateCreateInfo viewportState =
		vk::initializers::PipelineViewportStateCreateInfo(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisampleState =
		vk::initializers::PipelineMultisampleStateCreateInfo(
			VK_SAMPLE_COUNT_1_BIT,
			0);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState =
		vk::initializers::PipelineDynamicStateCreateInfo(
			dynamicStateEnables.data(),
			dynamicStateEnables.size(),
			0);

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		vk::initializers::GraphicsPipelineCreateInfo(
			m_resources.pipelineLayouts->get("scene"),
			m_context.renderPass,
			0);

	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();

	// G-Buffer Generation
	shaderStages[0] = LoadShader("shaders/screenTri.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = LoadShader("shaders/sceneSDF.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
		vk::initializers::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
		vk::initializers::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
		vk::initializers::PipelineColorBlendAttachmentState(0xf, VK_FALSE)
	};
	colorBlendState.attachmentCount = blendAttachmentStates.size();
	colorBlendState.pAttachments = blendAttachmentStates.data();

	VkPipelineVertexInputStateCreateInfo emptyInputState = vk::initializers::PipelineVertexInputStateCreateInfo();
	pipelineCreateInfo.pVertexInputState = &emptyInputState;
	pipelineCreateInfo.renderPass = m_framebuffers.offscreen.renderPass;
	pipelineCreateInfo.layout = m_resources.pipelineLayouts->get("scene");
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
	depthStencilState.depthWriteEnable = VK_TRUE;
	m_resources.pipelines->add("scene", pipelineCreateInfo, m_pipelineCache);

	// Deferred lighting pipeline
	shaderStages[1] = VkRenderBase::LoadShader("shaders/deferred.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	colorBlendState.pAttachments = &blendAttachmentState;
	colorBlendState.attachmentCount = 1;
	pipelineCreateInfo.layout = m_resources.pipelineLayouts->get("lights");
	pipelineCreateInfo.renderPass = m_framebuffers.lights.renderPass;
	m_resources.pipelines->add("lights", pipelineCreateInfo, m_pipelineCache);

	// Radial blur pipeline
	shaderStages[1] = VkRenderBase::LoadShader("shaders/radialblur.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	// Empty vertex input state
	pipelineCreateInfo.renderPass = m_context.renderPass;
	pipelineCreateInfo.pVertexInputState = &emptyInputState;
	pipelineCreateInfo.layout = m_resources.pipelineLayouts->get("blur");
	// Additive blending
	blendAttachmentState.colorWriteMask = 0xF;
	blendAttachmentState.blendEnable = VK_TRUE;
	m_resources.pipelines->add("blur", pipelineCreateInfo, m_pipelineCache);
}

// Prepare and initialize uniform buffer containing shader uniforms
void VkRenderBackend::PrepareUniformBuffers()
{
	// Phong and color pass vertex shader uniform buffer
	m_device->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(uboScene),
		&m_uniformBuffers.scene,
		&uboScene);

	// Fullscreen radial blur parameters
	m_device->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(uboBlur),
		&m_uniformBuffers.blur,
		&uboBlur);

	// Fullscreen radial blur parameters
	m_device->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(UBOLight),
		&m_uniformBuffers.lights,
		&uboLights);

	// Map persistent
	m_uniformBuffers.scene.map();
	m_uniformBuffers.blur.map();
	m_uniformBuffers.lights.map();
}

void VkRenderBackend::RequestFrameRender()
{
	vkQueueWaitIdle(m_context.presentQueue);

	Draw();
}

void VkRenderBackend::UpdateUniformBuffers(Camera camera, glm::vec3* deformVecs, float dt)
{
	// Scene rendering
	uboScene.view = camera.getView();
	uboScene.fov = 45.0f;
	uboScene.eye = camera.getPos();
	uboScene.resolution = glm::vec2(m_window->width, m_window->height);
	uboScene.dt = dt;

	if (!m_uniformBuffers.scene.mapped)
		m_uniformBuffers.scene.map();

	m_uniformBuffers.scene.copyTo(&uboScene, sizeof(UBOScene));
	m_uniformBuffers.scene.unmap();

	// Deferred lighting
	uboLights.view = camera.getView();
	uboLights.eye = glm::vec4(camera.getPos(), 0.0f);
	uboLights.model = glm::mat4();

	if (!m_uniformBuffers.lights.mapped)
		m_uniformBuffers.lights.map();

	m_uniformBuffers.lights.copyTo(&uboScene, sizeof(UBOScene));
	m_uniformBuffers.lights.unmap();
}

#pragma region Vulkan Functions

void VkRenderBackend::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_device->logicalDevice, &layoutInfo, nullptr, m_resources.descriptorSetLayouts->getPtr("scene")) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	layoutInfo.bindingCount = 2;

	if (vkCreateDescriptorSetLayout(m_device->logicalDevice, &layoutInfo, nullptr, m_resources.descriptorSetLayouts->getPtr("blur")) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

//void VkRenderBackend::CreateTextureImage()
//{
//	int32_t texWidth, texHeight, texChannels;
//	stbi_uc* pixels = stbi_load("../source/assets/textures/TestTexture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
//	VkDeviceSize imageSize = texWidth * texHeight * 4;
//
//	if (!pixels)
//	{
//		throw std::runtime_error("failed to load texture image");
//	}
//
//	VkBuffer stagingBuffer;
//	VkDeviceMemory stagingBufferMemory;
//	m_device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, imageSize, &stagingBuffer, &stagingBufferMemory);
//
//	void* data;
//	vkMapMemory(m_device->logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
//	memcpy(data, pixels, static_cast<size_t>(imageSize));
//	vkUnmapMemory(m_device->logicalDevice, stagingBufferMemory);
//
//	stbi_image_free(pixels);
//
//	m_device->CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);
//
//	m_device->TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_context.graphicsQueue);
//	m_device->CopyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), m_context.graphicsQueue);
//	m_device->TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_context.graphicsQueue);
//
//	vkDestroyBuffer(m_device->logicalDevice, stagingBuffer, nullptr);
//	vkFreeMemory(m_device->logicalDevice, stagingBufferMemory, nullptr);
//}

//void VkRenderBackend::CreateTextureImageView()
//{
//	VkImageViewCreateInfo viewInfo = {};
//	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//	viewInfo.image = m_textureImage;
//	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
//	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//	viewInfo.subresourceRange.baseMipLevel = 0;
//	viewInfo.subresourceRange.levelCount = 1;
//	viewInfo.subresourceRange.baseArrayLayer = 0;
//	viewInfo.subresourceRange.layerCount = 1;
//
//	if (vkCreateImageView(m_device->logicalDevice, &viewInfo, nullptr, &m_textureImageView) != VK_SUCCESS)
//	{
//		throw std::runtime_error("failed to create texture image view!");
//	}
//}

//void VkRenderBackend::CreateTextureSampler()
//{
//	VkSamplerCreateInfo samplerInfo = {};
//	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//	samplerInfo.magFilter = VK_FILTER_LINEAR;
//	samplerInfo.minFilter = VK_FILTER_LINEAR;
//	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//	samplerInfo.anisotropyEnable = VK_TRUE;
//	samplerInfo.maxAnisotropy = 16;
//	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
//	samplerInfo.unnormalizedCoordinates = VK_FALSE;
//	samplerInfo.compareEnable = VK_FALSE;
//	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
//	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//	samplerInfo.mipLodBias = 0.0f;
//	samplerInfo.minLod = 0.0f;
//	samplerInfo.maxLod = 0.0f;
//
//	if (vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
//	{
//		throw std::runtime_error("failed to create texture sampler!");
//	}
//}

void VkRenderBackend::Draw()
{
	VkRenderBase::PrepareFrame();

	// Offscreen rendering

	// Wait for swap chain presentation to finish
	m_submitInfo.pWaitSemaphores = &m_semaphores.imageAvailable;
	// Signal ready with offscreen semaphore
	m_submitInfo.pSignalSemaphores = &m_offscreenSemaphore;

	// Submit work
	m_submitInfo.commandBufferCount = 1;
	m_submitInfo.pCommandBuffers = &m_offscreenCommandBuffer;
	vkQueueSubmit(m_context.graphicsQueue, 1, &m_submitInfo, VK_NULL_HANDLE);

	// Scene rendering

	// Wait for offscreen semaphore
	m_submitInfo.pWaitSemaphores = &m_offscreenSemaphore;
	// Signal ready with render complete semaphpre
	m_submitInfo.pSignalSemaphores = &m_semaphores.renderFinished;

	// Submit work
	m_submitInfo.pCommandBuffers = &m_commandBuffers[m_currentBuffer];
	vkQueueSubmit(m_context.graphicsQueue, 1, &m_submitInfo, VK_NULL_HANDLE);

	VkRenderBase::SubmitFrame();
}

void VkRenderBackend::SetupLights()
{
	// 5 fixed lights
	std::array<glm::vec3, 5> lightColors;
	lightColors[0] = glm::vec3(1.0f, 0.0f, 0.0f);
	lightColors[1] = glm::vec3(1.0f, 0.7f, 0.7f);
	lightColors[2] = glm::vec3(1.0f, 0.0f, 0.0f);
	lightColors[3] = glm::vec3(0.0f, 0.0f, 1.0f);
	lightColors[4] = glm::vec3(1.0f, 0.0f, 0.0f);

	for (int32_t i = 0; i < lightColors.size(); i++)
	{
		SetupLight(&uboLights.lights[i], glm::vec3((float)(i - 2.5f) * 50.0f, -10.0f, 0.0f), lightColors[i], 120.0f);
	}

	// Dynamic light moving over the floor
	SetupLight(&uboLights.lights[0], { -sin(glm::radians(360.0f)) * 120.0f, -2.5f, cos(glm::radians(360.0f * 8.0f)) * 10.0f }, glm::vec3(1.0f), 100.0f);

	SetupLight(&uboLights.lights[5], { -48.75f, -16.0f, -17.8f }, { 1.0f, 0.6f, 0.0f }, 45.0f);
	SetupLight(&uboLights.lights[6], { -48.75f, -16.0f,  18.4f }, { 1.0f, 0.6f, 0.0f }, 45.0f);
	SetupLight(&uboLights.lights[7], { 62.0f, -16.0f, -17.8f }, { 1.0f, 0.6f, 0.0f }, 45.0f);
	SetupLight(&uboLights.lights[8], { 62.0f, -16.0f,  18.4f }, { 1.0f, 0.6f, 0.0f }, 45.0f);
	
	SetupLight(&uboLights.lights[9], { 120.0f, -20.0f, -43.75f }, { 1.0f, 0.8f, 0.3f }, 75.0f);
	SetupLight(&uboLights.lights[10], { 120.0f, -20.0f, 41.75f }, { 1.0f, 0.8f, 0.3f }, 75.0f);
	SetupLight(&uboLights.lights[11], { -110.0f, -20.0f, -43.75f }, { 1.0f, 0.8f, 0.3f }, 75.0f);
	SetupLight(&uboLights.lights[12], { -110.0f, -20.0f, 41.75f }, { 1.0f, 0.8f, 0.3f }, 75.0f);

	SetupLight(&uboLights.lights[13], { -122.0f, -18.0f, -3.2f }, { 1.0f, 0.3f, 0.3f }, 25.0f);
	SetupLight(&uboLights.lights[14], { -122.0f, -18.0f,  3.2f }, { 0.3f, 1.0f, 0.3f }, 25.0f);
	
	SetupLight(&uboLights.lights[15], { 135.0f, -18.0f, -3.2f }, { 0.3f, 0.3f, 1.0f }, 25.0f);
	SetupLight(&uboLights.lights[16], { 135.0f, -18.0f,  3.2f }, { 1.0f, 1.0f, 0.3f }, 25.0f);
}

#pragma endregion

#pragma region Helper Functions

void VkRenderBackend::SetupLight(Light *light, glm::vec3 pos, glm::vec3 color, float radius)
{
	light->position = glm::vec4(pos, 1.0f);
	light->color = glm::vec4(color, 1.0f);
	light->radius = radius;
	// linear and quadratic falloff not used with new shader
}

#pragma endregion
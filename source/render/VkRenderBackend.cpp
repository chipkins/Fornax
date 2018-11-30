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

//struct Resources
//{
//	vk::PipelineLayoutList* pipelineLayouts;
//	vk::PipelineList *pipelines;
//	vk::DescriptorSetLayoutList *descriptorSetLayouts;
//	vk::DescriptorSetList * descriptorSets;
//} resources;

void VkRenderBackend::Init(GLFWwindow* window)
{
	VkRenderBase::Init(window);

	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();

	m_models.emplace_back();
	m_models[0].LoadModel("../source/assets/models/quad.obj");
	CreateVertexBuffer();
	CreateIndexBuffer();
	SetupVertexInput();

	SetupDescriptorPool();
	PrepareOffscreenFramebuffer();
	PrepareUniformBuffers();
	SetupLayoutsAndDescriptors();
	PreparePipelines();
	BuildCommandBuffers();
	BuildOffscreenCommandBuffer();
}

void VkRenderBackend::Cleanup()
{
	m_uniformBuffers.blur.destroy();
	m_uniformBuffers.scene.destroy();

	vkDestroyImageView(m_device->logicalDevice, m_textureImageView, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_textureImage, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_textureImageMemory, nullptr);
	vkDestroySampler(m_device->logicalDevice, m_textureSampler, nullptr);

	// Color attachment
	vkDestroyImageView(m_device->logicalDevice, m_offScreenFrameBuffer.color.view, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_offScreenFrameBuffer.color.image, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_offScreenFrameBuffer.color.memory, nullptr);

	// Depth attachment
	vkDestroyImageView(m_device->logicalDevice, m_offScreenFrameBuffer.depth.view, nullptr);
	vkDestroyImage(m_device->logicalDevice, m_offScreenFrameBuffer.depth.image, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_offScreenFrameBuffer.depth.memory, nullptr);

	vkFreeCommandBuffers(m_device->logicalDevice, m_commandPool, 1, &m_offScreenFrameBuffer.commandBuffer);
	vkDestroyRenderPass(m_device->logicalDevice, m_offScreenFrameBuffer.renderPass, nullptr);
	vkDestroySemaphore(m_device->logicalDevice, m_offScreenFrameBuffer.semaphore, nullptr);
	vkDestroyFramebuffer(m_device->logicalDevice, m_offScreenFrameBuffer.frameBuffer, nullptr);
	vkDestroySampler(m_device->logicalDevice, m_offScreenFrameBuffer.sampler, nullptr);

	vkDestroyPipeline(m_device->logicalDevice, m_pipelines.blur, nullptr);
	vkDestroyPipeline(m_device->logicalDevice, m_pipelines.scene, nullptr);

	vkDestroyPipelineLayout(m_device->logicalDevice, m_pipelineLayouts.blur, nullptr);
	vkDestroyPipelineLayout(m_device->logicalDevice, m_pipelineLayouts.scene, nullptr);

	vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayouts.blur, nullptr);
	vkDestroyDescriptorSetLayout(m_device->logicalDevice, m_descriptorSetLayouts.scene, nullptr);

	vkDestroyBuffer(m_device->logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_vertexBufferMemory, nullptr);
	vkDestroyBuffer(m_device->logicalDevice, m_indexBuffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_indexBufferMemory, nullptr);

	VkRenderBase::Cleanup();
}

// Setup the offscreen framebuffer for rendering the blurred scene
// The color attachment of this framebuffer will then be used to sample frame in the fragment shader of the final pass
void VkRenderBackend::PrepareOffscreenFramebuffer()
{
	m_offScreenFrameBuffer.width = 512;
	m_offScreenFrameBuffer.height = 512;

	// Find a suitable depth format
	VkFormat depthFormat = FindDepthFormat();

	// Color attachment
	VkImageCreateInfo image = vk::initializers::ImageCreateInfo();
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = VK_FORMAT_R8G8B8A8_UNORM;
	image.extent.width = m_offScreenFrameBuffer.width;
	image.extent.height = m_offScreenFrameBuffer.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	// We will sample directly from the color attachment
	image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkMemoryAllocateInfo memAlloc = vk::initializers::MemoryAllocateInfo();
	VkMemoryRequirements memReqs;

	vkCreateImage(m_device->logicalDevice, &image, nullptr, &m_offScreenFrameBuffer.color.image);
	vkGetImageMemoryRequirements(m_device->logicalDevice, m_offScreenFrameBuffer.color.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = m_device->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(m_device->logicalDevice, &memAlloc, nullptr, &m_offScreenFrameBuffer.color.memory);
	vkBindImageMemory(m_device->logicalDevice, m_offScreenFrameBuffer.color.image, m_offScreenFrameBuffer.color.memory, 0);

	VkImageViewCreateInfo colorImageView = vk::initializers::ImageViewCreateInfo();
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.format = VK_FORMAT_R8G8B8A8_UNORM;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;
	colorImageView.image = m_offScreenFrameBuffer.color.image;
	vkCreateImageView(m_device->logicalDevice, &colorImageView, nullptr, &m_offScreenFrameBuffer.color.view);

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
	vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_offScreenFrameBuffer.sampler);

	// Depth stencil attachment
	image.format = depthFormat;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	vkCreateImage(m_device->logicalDevice, &image, nullptr, &m_offScreenFrameBuffer.depth.image);
	vkGetImageMemoryRequirements(m_device->logicalDevice, m_offScreenFrameBuffer.depth.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = m_device->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(m_device->logicalDevice, &memAlloc, nullptr, &m_offScreenFrameBuffer.depth.memory);
	vkBindImageMemory(m_device->logicalDevice, m_offScreenFrameBuffer.depth.image, m_offScreenFrameBuffer.depth.memory, 0);

	VkImageViewCreateInfo depthStencilView = vk::initializers::ImageViewCreateInfo();
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = m_offScreenFrameBuffer.depth.image;
	vkCreateImageView(m_device->logicalDevice, &depthStencilView, nullptr, &m_offScreenFrameBuffer.depth.view);

	// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

	std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
	// Color attachment
	attchmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
	attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	// Depth attachment
	attchmentDescriptions[1].format = depthFormat;
	attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
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

	vkCreateRenderPass(m_device->logicalDevice, &renderPassInfo, nullptr, &m_offScreenFrameBuffer.renderPass);

	VkImageView attachments[2];
	attachments[0] = m_offScreenFrameBuffer.color.view;
	attachments[1] = m_offScreenFrameBuffer.depth.view;

	VkFramebufferCreateInfo fbufCreateInfo = vk::initializers::FramebufferCreateInfo();
	fbufCreateInfo.renderPass = m_offScreenFrameBuffer.renderPass;
	fbufCreateInfo.attachmentCount = 2;
	fbufCreateInfo.pAttachments = attachments;
	fbufCreateInfo.width = m_offScreenFrameBuffer.width;
	fbufCreateInfo.height = m_offScreenFrameBuffer.height;
	fbufCreateInfo.layers = 1;

	vkCreateFramebuffer(m_device->logicalDevice, &fbufCreateInfo, nullptr, &m_offScreenFrameBuffer.frameBuffer);

	// Fill a descriptor for later use in a descriptor set 
	m_offScreenFrameBuffer.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_offScreenFrameBuffer.descriptor.imageView = m_offScreenFrameBuffer.color.view;
	m_offScreenFrameBuffer.descriptor.sampler = m_offScreenFrameBuffer.sampler;
}

// Sets up the command buffer that renders the scene to the offscreen frame buffer
void VkRenderBackend::BuildOffscreenCommandBuffer()
{
	if (m_offScreenFrameBuffer.commandBuffer == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vk::initializers::CommandBufferAllocateInfo(
				m_commandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		vkAllocateCommandBuffers(m_device->logicalDevice, &cmdBufAllocateInfo, &m_offScreenFrameBuffer.commandBuffer);
	}
	if (m_offScreenFrameBuffer.semaphore == VK_NULL_HANDLE)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = vk::initializers::SemaphoreCreateInfo();
		vkCreateSemaphore(m_device->logicalDevice, &semaphoreCreateInfo, nullptr, &m_offScreenFrameBuffer.semaphore);
	}

	VkCommandBufferBeginInfo cmdBufInfo = vk::initializers::CommandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vk::initializers::RenderPassBeginInfo();
	renderPassBeginInfo.renderPass = m_offScreenFrameBuffer.renderPass;
	renderPassBeginInfo.framebuffer = m_offScreenFrameBuffer.frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = m_offScreenFrameBuffer.width;
	renderPassBeginInfo.renderArea.extent.height = m_offScreenFrameBuffer.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	vkBeginCommandBuffer(m_offScreenFrameBuffer.commandBuffer, &cmdBufInfo);

	VkViewport viewport = vk::initializers::Viewport((float)m_offScreenFrameBuffer.width, (float)m_offScreenFrameBuffer.height, 0.0f, 1.0f);
	vkCmdSetViewport(m_offScreenFrameBuffer.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = vk::initializers::Rect2D(m_offScreenFrameBuffer.width, m_offScreenFrameBuffer.height, 0, 0);
	vkCmdSetScissor(m_offScreenFrameBuffer.commandBuffer, 0, 1, &scissor);

	vkCmdBeginRenderPass(m_offScreenFrameBuffer.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindDescriptorSets(m_offScreenFrameBuffer.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayouts.scene, 0, 1, &m_descriptorSets.scene, 0, NULL);
	vkCmdBindPipeline(m_offScreenFrameBuffer.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.scene);

	vkCmdDraw(m_offScreenFrameBuffer.commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(m_offScreenFrameBuffer.commandBuffer);

	vkEndCommandBuffer(m_offScreenFrameBuffer.commandBuffer);
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
		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayouts.scene, 0, 1, &m_descriptorSets.scene, 0, NULL);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.scene);

		/*vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, &m_vertexBuffer, offsets);
		vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_commandBuffers[i], m_models[0].getNumIndices(), 1, 0, 0, 0);*/

		// Fullscreen triangle (clipped to a quad) with radial blur
		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayouts.blur, 0, 1, &m_descriptorSets.blur, 0, NULL);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.blur);
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
		vk::initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo =
		vk::initializers::DescriptorPoolCreateInfo(
			poolSizes.size(),
			poolSizes.data(),
			2);

	vkCreateDescriptorPool(m_device->logicalDevice, &descriptorPoolInfo, nullptr, &m_descriptorPool);
}

void VkRenderBackend::SetupLayoutsAndDescriptors()
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
	VkDescriptorSetLayoutCreateInfo descriptorLayout;
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo;

	// Scene rendering
	setLayoutBindings =
	{
		// Binding 0: Vertex shader uniform buffer
		vk::initializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0),
	};
	descriptorLayout = vk::initializers::DescriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
	vkCreateDescriptorSetLayout(m_device->logicalDevice, &descriptorLayout, nullptr, &m_descriptorSetLayouts.scene);
	pPipelineLayoutCreateInfo = vk::initializers::PipelineLayoutCreateInfo(&m_descriptorSetLayouts.scene, 1);
	vkCreatePipelineLayout(m_device->logicalDevice, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayouts.scene);

	// Fullscreen radial blur
	setLayoutBindings =
	{
		// Binding 0 : Fragment shader uniform buffer
		vk::initializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0),
		// Binding 0: Fragment shader image sampler
		vk::initializers::DescriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			1)
	};
	descriptorLayout = vk::initializers::DescriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
	vkCreateDescriptorSetLayout(m_device->logicalDevice, &descriptorLayout, nullptr, &m_descriptorSetLayouts.blur);
	pPipelineLayoutCreateInfo = vk::initializers::PipelineLayoutCreateInfo(&m_descriptorSetLayouts.blur, 1);
	vkCreatePipelineLayout(m_device->logicalDevice, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayouts.blur);

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo;

	// Scene rendering
	descriptorSetAllocInfo = vk::initializers::DescriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayouts.scene, 1);
	vkAllocateDescriptorSets(m_device->logicalDevice, &descriptorSetAllocInfo, &m_descriptorSets.scene);

	std::vector<VkWriteDescriptorSet> offScreenWriteDescriptorSets =
	{
		// Binding 0: Vertex shader uniform buffer
		vk::initializers::WriteDescriptorSet(
			m_descriptorSets.scene,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			0,
			&m_uniformBuffers.scene.descriptor),
	};
	vkUpdateDescriptorSets(m_device->logicalDevice, offScreenWriteDescriptorSets.size(), offScreenWriteDescriptorSets.data(), 0, NULL);

	// Fullscreen radial blur
	descriptorSetAllocInfo = vk::initializers::DescriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayouts.blur, 1);
	vkAllocateDescriptorSets(m_device->logicalDevice, &descriptorSetAllocInfo, &m_descriptorSets.blur);

	std::vector<VkWriteDescriptorSet> writeDescriptorSets =
	{
		// Binding 0: Vertex shader uniform buffer
		vk::initializers::WriteDescriptorSet(
			m_descriptorSets.blur,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			0,
			&m_uniformBuffers.blur.descriptor),
		// Binding 0: Fragment shader texture sampler
		vk::initializers::WriteDescriptorSet(
			m_descriptorSets.blur,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			&m_offScreenFrameBuffer.descriptor),
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
			VK_CULL_MODE_NONE,
			VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
			m_pipelineLayouts.blur,
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

	// Radial blur pipeline
	shaderStages[0] = VkRenderBase::LoadShader("shaders/screenTri.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = VkRenderBase::LoadShader("shaders/radialblur.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	// Empty vertex input state
	VkPipelineVertexInputStateCreateInfo emptyInputState = vk::initializers::PipelineVertexInputStateCreateInfo();
	pipelineCreateInfo.pVertexInputState = &emptyInputState;
	//pipelineCreateInfo.layout = m_pipelineLayouts.blur;
	// Additive blending
	blendAttachmentState.colorWriteMask = 0xF;
	blendAttachmentState.blendEnable = VK_TRUE;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
	vkCreateGraphicsPipelines(m_device->logicalDevice, m_pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_pipelines.blur);

	// Color only pass (offscreen blur base)
	shaderStages[0] = LoadShader("shaders/screenTri.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = LoadShader("shaders/sceneSDF.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineCreateInfo.pVertexInputState = &emptyInputState;
	pipelineCreateInfo.renderPass = m_offScreenFrameBuffer.renderPass;
	pipelineCreateInfo.layout = m_pipelineLayouts.scene;
	blendAttachmentState.blendEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	vkCreateGraphicsPipelines(m_device->logicalDevice, m_pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_pipelines.scene);
}

// Prepare and initialize uniform buffer containing shader uniforms
void VkRenderBackend::PrepareUniformBuffers()
{
	// Phong and color pass vertex shader uniform buffer
	m_device->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(uboScene),
		&m_uniformBuffers.scene);

	// Fullscreen radial blur parameters
	m_device->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(uboBlur),
		&m_uniformBuffers.blur,
		&uboBlur);

	// Map persistent
	m_uniformBuffers.scene.map();
	m_uniformBuffers.blur.map();
}

void VkRenderBackend::SetupVertexInput()
{
	vertexInput.attributeDescriptions = vk::Vertex::GetAttributeDescriptions();
	vertexInput.bindingDescriptions.push_back(vk::Vertex::GetBindingDescription());

	vertexInput.inputState = vk::initializers::PipelineVertexInputStateCreateInfo();
	vertexInput.inputState.vertexBindingDescriptionCount = vertexInput.bindingDescriptions.size();
	vertexInput.inputState.pVertexBindingDescriptions = vertexInput.bindingDescriptions.data();
	vertexInput.inputState.vertexAttributeDescriptionCount = vertexInput.attributeDescriptions.size();
	vertexInput.inputState.pVertexAttributeDescriptions = vertexInput.attributeDescriptions.data();
}

void VkRenderBackend::RequestFrameRender()
{
	vkQueueWaitIdle(m_context.presentQueue);

	Draw();
}

void VkRenderBackend::UpdateUniformBuffers(Camera camera, glm::vec3* deformVecs, float dt)
{
	uboScene.view = camera.getView();
	uboScene.fov = 45.0f;
	uboScene.eye = camera.getPos();
	uboScene.resolution = glm::vec2(m_window->width, m_window->height);
	uboScene.dt = dt;

	if (!m_uniformBuffers.scene.mapped)
		m_uniformBuffers.scene.map();

	m_uniformBuffers.scene.copyTo(&uboScene, sizeof(UBOScene));
	m_uniformBuffers.scene.unmap();
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

	if (vkCreateDescriptorSetLayout(m_device->logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayouts.scene) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	layoutInfo.bindingCount = 2;

	if (vkCreateDescriptorSetLayout(m_device->logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayouts.blur) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VkRenderBackend::CreateTextureImage()
{
	int32_t texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("../source/assets/textures/TestTexture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	m_device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, imageSize, &stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(m_device->logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_device->logicalDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	m_device->CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);

	m_device->TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_context.graphicsQueue);
	m_device->CopyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), m_context.graphicsQueue);
	m_device->TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_context.graphicsQueue);

	vkDestroyBuffer(m_device->logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, stagingBufferMemory, nullptr);
}

void VkRenderBackend::CreateTextureImageView()
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_device->logicalDevice, &viewInfo, nullptr, &m_textureImageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}
}

void VkRenderBackend::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(m_device->logicalDevice, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void VkRenderBackend::CreateVertexBuffer()
{
	std::vector<vk::Vertex> vertices;
	for (auto& model : m_models)
	{
		for (int i = 0; i < model.getNumVertices(); ++i)
		{
			vertices.push_back(model.getVertices()[i]);
		}
	}

	VkDeviceSize bufferSize = sizeof(vk::Vertex) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	m_device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferSize, &stagingBuffer, &stagingBufferMemory);
	void* data;
	vkMapMemory(m_device->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_device->logicalDevice, stagingBufferMemory);

	m_device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferSize, &m_vertexBuffer, &m_vertexBufferMemory);

	m_device->CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize, m_context.graphicsQueue);

	vkDestroyBuffer(m_device->logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, stagingBufferMemory, nullptr);
}

void VkRenderBackend::CreateIndexBuffer()
{
	std::vector<uint32_t> indicies;
	for (auto& model : m_models)
	{
		for (int i = 0; i < model.getNumIndices(); ++i)
		{
			indicies.push_back(model.getIndices()[i]);
		}
	}

	VkDeviceSize bufferSize = sizeof(uint32_t) * indicies.size();
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	m_device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferSize, &stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(m_device->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indicies.data(), (size_t)bufferSize);
	vkUnmapMemory(m_device->logicalDevice, stagingBufferMemory);

	m_device->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferSize, &m_indexBuffer, &m_indexBufferMemory);

	m_device->CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize, m_context.graphicsQueue);

	vkDestroyBuffer(m_device->logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, stagingBufferMemory, nullptr);
}

void VkRenderBackend::Draw()
{
	VkRenderBase::PrepareFrame();

	// Offscreen rendering

	// Wait for swap chain presentation to finish
	m_submitInfo.pWaitSemaphores = &m_semaphores.imageAvailable;
	// Signal ready with offscreen semaphore
	m_submitInfo.pSignalSemaphores = &m_offScreenFrameBuffer.semaphore;

	// Submit work
	m_submitInfo.commandBufferCount = 1;
	m_submitInfo.pCommandBuffers = &m_offScreenFrameBuffer.commandBuffer;
	vkQueueSubmit(m_context.graphicsQueue, 1, &m_submitInfo, VK_NULL_HANDLE);

	// Scene rendering

	// Wait for offscreen semaphore
	m_submitInfo.pWaitSemaphores = &m_offScreenFrameBuffer.semaphore;
	// Signal ready with render complete semaphpre
	m_submitInfo.pSignalSemaphores = &m_semaphores.renderFinished;

	// Submit work
	m_submitInfo.pCommandBuffers = &m_commandBuffers[m_currentBuffer];
	vkQueueSubmit(m_context.graphicsQueue, 1, &m_submitInfo, VK_NULL_HANDLE);

	VkRenderBase::SubmitFrame();
}

void VkRenderBackend::Prepare()
{
	SetupDescriptorPool();
	PrepareOffscreenFramebuffer();
	PrepareUniformBuffers();
	SetupLayoutsAndDescriptors();
	PreparePipelines();
	BuildCommandBuffers();
	BuildOffscreenCommandBuffer();
}

#pragma endregion

#pragma region Helper Functions

#pragma endregion
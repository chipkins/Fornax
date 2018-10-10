#pragma once

#include "vulkan\vulkan.h"

namespace vk
{
	namespace initializers
	{
		VkMemoryAllocateInfo MemoryAllocateInfo()
		{
			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.pNext = nullptr;
			memAllocInfo.allocationSize = 0;
			memAllocInfo.memoryTypeIndex = 0;
			return memAllocInfo;
		}

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t bufferCount)
		{
			VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
			cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufAllocInfo.commandPool = commandPool;
			cmdBufAllocInfo.level = level;
			cmdBufAllocInfo.commandBufferCount = bufferCount;
			return cmdBufAllocInfo;
		}

		VkCommandPoolCreateInfo CommandPoolCreateInfo()
		{
			VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
			cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			return cmdPoolCreateInfo;
		}

		VkCommandBufferBeginInfo CommandBufferBeginInfo()
		{
			VkCommandBufferBeginInfo cmdBufBeginInfo = {};
			cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufBeginInfo.pNext = nullptr;
			return cmdBufBeginInfo;
		}

		VkCommandBufferInheritanceInfo CommandBufferInheritanceInfo()
		{
			VkCommandBufferInheritanceInfo cmdBufInheritanceInfo = {};
			cmdBufInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			return cmdBufInheritanceInfo;
		}

		VkRenderPassBeginInfo RenderPassBegininfo()
		{
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			return renderPassBeginInfo;
		}

		VkRenderPassCreateInfo RenderPassCreateInfo()
		{
			VkRenderPassCreateInfo renderPassCreateInfo = {};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.pNext = nullptr;
			return renderPassCreateInfo;
		}

		VkImageMemoryBarrier ImageMemoryBarrier()
		{
			VkImageMemoryBarrier imgMemBarrier = {};
			imgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imgMemBarrier.pNext = nullptr;
			imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return imgMemBarrier;
		}

		VkBufferMemoryBarrier BufferMemoryBarrier()
		{
			VkBufferMemoryBarrier bufMemBarrier = {};
			bufMemBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			bufMemBarrier.pNext = nullptr;
			return bufMemBarrier;
		}

		VkMemoryBarrier MemoryBarrier()
		{
			VkMemoryBarrier memBarrier = {};
			memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memBarrier.pNext = nullptr;
			return memBarrier;
		}

		VkImageCreateInfo ImageCreateInfo()
		{
			VkImageCreateInfo imgCreateInfo = {};
			imgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imgCreateInfo.pNext = nullptr;
			return imgCreateInfo;
		}

		VkSamplerCreateInfo SamplerCreateInfo()
		{
			VkSamplerCreateInfo samplerCreateInfo = {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.pNext = nullptr;
			return samplerCreateInfo;
		}

		VkImageViewCreateInfo ImageViewCreateInfo()
		{
			VkImageViewCreateInfo imgViewCreateInfo = {};
			imgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imgViewCreateInfo.pNext = nullptr;
			return imgViewCreateInfo;
		}

		VkFramebufferCreateInfo FramebufferCreateInfo()
		{
			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = nullptr;
			return framebufferCreateInfo;
		}

		VkSemaphoreCreateInfo SemaphoreCreateInfo()
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			semaphoreCreateInfo.flags = 0;
			return semaphoreCreateInfo;
		}

		VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags)
		{
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = flags;
			return fenceCreateInfo;
		}

		VkEventCreateInfo EventCreateInfo()
		{
			VkEventCreateInfo eventCreateInfo = {};
			eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
			return eventCreateInfo;
		}

		VkSubmitInfo SubmitInfo()
		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			return submitInfo;
		}

		VkViewport Viewport(float width, float height, float minDepth, float maxDepth)
		{
			VkViewport viewport = {};
			viewport.width = width;
			viewport.height = height;
			viewport.minDepth = minDepth;
			viewport.maxDepth = maxDepth;
			return viewport;
		}

		VkRect2D Rect2D(int32_t width, int32_t height, int32_t offsetX, int32_t offsetY)
		{
			VkRect2D rect2D = {};
			rect2D.extent.width = width;
			rect2D.extent.height = height;
			rect2D.offset.x = offsetX;
			rect2D.offset.y = offsetY;
			return rect2D;
		}

		VkBufferCreateInfo BufferCreateInfo()
		{
			VkBufferCreateInfo bufCreateInfo = {};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			return bufCreateInfo;
		}

		VkBufferCreateInfo BufferCreateInfo(VkBufferUsageFlags usage, VkDeviceSize size)
		{
			VkBufferCreateInfo bufCreateInfo = {};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufCreateInfo.pNext = nullptr;
			bufCreateInfo.usage = usage;
			bufCreateInfo.size = size;
			bufCreateInfo.flags = 0;
			return bufCreateInfo;
		}

		VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(uint32_t poolSizeCount, VkDescriptorPoolSize* pPoolSizes, uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.pNext = nullptr;
			descriptorPoolInfo.poolSizeCount = poolSizeCount;
			descriptorPoolInfo.pPoolSizes = pPoolSizes;
			descriptorPoolInfo.maxSets = maxSets;
			return descriptorPoolInfo;
		}

		VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount)
		{
			VkDescriptorPoolSize descriptorPoolSize = {};
			descriptorPoolSize.type = type;
			descriptorPoolSize.descriptorCount = descriptorCount;
			return descriptorPoolSize;
		}

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stateFlags, uint32_t binding, uint32_t count)
		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.descriptorType = type;
			setLayoutBinding.stageFlags = stateFlags;
			setLayoutBinding.binding = binding;
			setLayoutBinding.descriptorCount = count;
			return setLayoutBinding;
		}

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding* pBindings, uint32_t bindingCount)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pNext = nullptr;
			descriptorSetLayoutCreateInfo.pBindings = pBindings;
			descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
			return descriptorSetLayoutCreateInfo;
		}

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(const VkDescriptorSetLayout* pSetLayouts, uint32_t setLayoutCount)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.pNext = nullptr;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
			return pipelineLayoutCreateInfo;
		}

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(uint32_t setLayoutCount)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			return pipelineLayoutCreateInfo;
		}

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(VkDescriptorPool descriptorPool, const VkDescriptorSetLayout* pSetLayouts, uint32_t descriptorSetCount)
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.pNext = nullptr;
			descriptorSetAllocateInfo.descriptorPool = descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
			descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
			return descriptorSetAllocateInfo;
		}

		VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
		{
			VkDescriptorImageInfo descriptorImageInfo = {};
			descriptorImageInfo.sampler = sampler;
			descriptorImageInfo.imageView = imageView;
			descriptorImageInfo.imageLayout = imageLayout;
			return descriptorImageInfo;
		}

		VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
		{
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.pNext = nullptr;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pBufferInfo = bufferInfo;
			writeDescriptorSet.descriptorCount = 1;
			return writeDescriptorSet;
		}

		VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo)
		{
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.pNext = nullptr;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pImageInfo = imageInfo;
			writeDescriptorSet.descriptorCount = 1;
			return writeDescriptorSet;
		}

		VkVertexInputBindingDescription VertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
		{
			VkVertexInputBindingDescription vertexInputBindingDescription = {};
			vertexInputBindingDescription.binding = binding;
			vertexInputBindingDescription.stride = stride;
			vertexInputBindingDescription.inputRate = inputRate;
			return vertexInputBindingDescription;
		}

		VkVertexInputAttributeDescription VertexInputAttributeDescription(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset)
		{
			VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
			vertexInputAttributeDescription.binding = binding;
			vertexInputAttributeDescription.location = location;
			vertexInputAttributeDescription.format = format;
			vertexInputAttributeDescription.offset = offset;
			return vertexInputAttributeDescription;
		}

		VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo()
		{
			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			pipelineVertexInputStateCreateInfo.pNext = nullptr;
			return pipelineVertexInputStateCreateInfo;
		}

		VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 primitiveRestartEnable)
		{
			VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
			pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			pipelineInputAssemblyStateCreateInfo.topology = topology;
			pipelineInputAssemblyStateCreateInfo.flags = flags;
			pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
			return pipelineInputAssemblyStateCreateInfo;
		}

		VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRasterizationStateCreateFlags flags)
		{
			VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
			pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
			pipelineRasterizationStateCreateInfo.cullMode = cullMode;
			pipelineRasterizationStateCreateInfo.frontFace = frontFace;
			pipelineRasterizationStateCreateInfo.flags = flags;
			pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
			return pipelineRasterizationStateCreateInfo;
		}

		VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask, VkBool32 blendEnable)
		{
			VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
			pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
			pipelineColorBlendAttachmentState.blendEnable = blendEnable;
			return pipelineColorBlendAttachmentState;
		}

		VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* pAttachments)
		{
			VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
			pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			pipelineColorBlendStateCreateInfo.pNext = nullptr;
			pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
			pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
			return pipelineColorBlendStateCreateInfo;
		}

		VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp)
		{
			VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
			pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
			pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
			pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
			pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;
			pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
			return pipelineDepthStencilStateCreateInfo;
		}

		VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags)
		{
			VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
			pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			pipelineViewportStateCreateInfo.viewportCount = viewportCount;
			pipelineViewportStateCreateInfo.scissorCount = scissorCount;
			pipelineViewportStateCreateInfo.flags = flags;
			return pipelineViewportStateCreateInfo;
		}

		VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo(VkSampleCountFlagBits rasterizationSamples, VkPipelineMultisampleStateCreateFlags flags)
		{
			VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
			pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
			pipelineMultisampleStateCreateInfo.flags = flags;
			return pipelineMultisampleStateCreateInfo;
		}

		VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(const VkDynamicState* pDynamicState, uint32_t dynamicStateCount, VkPipelineDynamicStateCreateFlags flags)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicState;
			pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
			pipelineDynamicStateCreateInfo.flags = flags;
			return pipelineDynamicStateCreateInfo;
		}

		VkPipelineTessellationStateCreateInfo PipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
		{
			VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo = {};
			pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
			return pipelineTessellationStateCreateInfo;
		}

		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo(VkPipelineLayout layout, VkRenderPass renderPass, VkPipelineCreateFlags flags)
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.pNext = nullptr;
			pipelineCreateInfo.layout = layout;
			pipelineCreateInfo.renderPass = renderPass;
			pipelineCreateInfo.flags = flags;
			return pipelineCreateInfo;
		}

		VkComputePipelineCreateInfo ComputePipelineCreateInfo(VkPipelineLayout layout, VkPipelineCreateFlags flags)
		{
			VkComputePipelineCreateInfo pipelineCreateInfo = {};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.layout = layout;
			pipelineCreateInfo.flags = flags;
			return pipelineCreateInfo;
		}

		VkPushConstantRange PushConstantRange(VkShaderStageFlags stateFlags, uint32_t size, uint32_t offset)
		{
			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = stateFlags;
			pushConstantRange.size = size;
			pushConstantRange.offset = offset;
			return pushConstantRange;
		}

		VkBindSparseInfo BindSparseInfo()
		{
			VkBindSparseInfo bindSparseInfo = {};
			bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
			return bindSparseInfo;
		}

		VkSpecializationMapEntry SpecializationMapEntry(uint32_t constantId, uint32_t size, uint32_t offset)
		{
			VkSpecializationMapEntry specializationEntry = {};
			specializationEntry.constantID = constantId;
			specializationEntry.size = size;
			specializationEntry.offset = offset;
			return specializationEntry;
		}

		VkSpecializationInfo SpecializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* pMapEntries, size_t dataSize, const void* pData)
		{
			VkSpecializationInfo specializationInfo = {};
			specializationInfo.mapEntryCount = mapEntryCount;
			specializationInfo.pMapEntries = pMapEntries;
			specializationInfo.dataSize = dataSize;
			specializationInfo.pData = pData;
			return specializationInfo;
		}
	}
}
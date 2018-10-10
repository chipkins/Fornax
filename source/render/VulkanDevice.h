#pragma once

#include "VulkanHeader.h"

namespace vk
{
	class Device
	{
	public:
		VkPhysicalDevice                     physicalDevice;
		VkDevice                             logicalDevice;
		VkPhysicalDeviceProperties           deviceProperties;
		VkPhysicalDeviceFeatures             deviceFeatures;
		VkPhysicalDeviceMemoryProperties     memoryProperties;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		std::vector<VkExtensionProperties>   extensionProperties;
		std::vector<const char*>             supportedExtensions;

		// Default command pool for the graphics queue
		VkCommandPool commandPool = VK_NULL_HANDLE;
		struct {
			uint32_t graphics;
			uint32_t present;
		} queueFamilyIndices;
		
		bool enableDebug = false;

		Device(std::vector<VkPhysicalDevice> physicalDevices)
		{
			for (const auto& device : physicalDevices)
			{
				if (IsDeviceSuitable(device))
				{
					physicalDevice = device;
					break;
				}
			}

			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
			vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
			extensionProperties.resize(extensionCount);
			if (extensionCount > 0)
			{
				if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionProperties.data()) == VK_SUCCESS)
				{
					for (auto extension : extensionProperties)
					{
						supportedExtensions.push_back(extension.extensionName);
					}
				}
			}
		}

		~Device()
		{
			if (commandPool)
			{
				vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
			}
			if (logicalDevice)
			{
				vkDestroyDevice(logicalDevice, nullptr);
			}
		}

	private:
		bool IsDeviceSuitable(VkPhysicalDevice)
		{
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
			vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

			bool isDiscrete = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

			// Find suitable gaphics and presentation queues
			uint32_t unassignedQueueValue = std::numeric_limits<uint32_t>::max();
			queueFamilyIndices.graphics = unassignedQueueValue;
			queueFamilyIndices.present = unassignedQueueValue;

			uint32_t queueCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
			if (queueCount < 1)
			{
				throw std::runtime_error("No Vulkan QueueFamilyProperties found");
			}
			queueFamilyProperties.resize(queueCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueFamilyProperties.data());

			uint32_t i = 0;
			for (const auto& queueFamily : queueFamilyProperties)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

				if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					queueFamilyIndices.graphics = i;
				}
				if (queueFamily.queueCount > 0 && presentSupport)
				{
					presentFamilyIndex = i;
				}
				if (queueFamilyIndices.graphics != unassignedQueueValue && presentFamilyIndex != unassignedQueueValue)
				{
					break;
				}

				++i;
			}

			if (graphicsFamilyIndex == unassignedQueueValue && presentFamilyIndex == unassignedQueueValue)
			{
				throw std::runtime_error("Cannot find a graphics and/or presenting queue");
			}
		}

		VkCommandBuffer BeginSingleTimeCommands()
		{
			VkCommandBufferAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocateInfo.commandPool = commandPool;
			allocateInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer;
			vkAllocateCommandBuffers(logicalDevice, &allocateInfo, &commandBuffer);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			return commandBuffer;
		}

		void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue)
		{
			vkEndCommandBuffer(commandBuffer);

			VkSubmitInfo submitInfo = vk::initializers::SubmitInfo();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(queue);

			vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
		}

		VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = 0)
		{
			VkCommandPoolCreateInfo cmdPoolInfo = vk::initializers::CommandPoolCreateInfo();
			cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
			cmdPoolInfo.flags = createFlags;

			VkCommandPool cmdPool;
			if (vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create command pool");
			}
			return cmdPool;
		}

	public:
		uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr)
		{
			for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
			{
				if ((typeBits & 1) == 1)
				{
					if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
					{
						if (memTypeFound)
						{
							*memTypeFound = true;
						}
						return i;
					}
				}
			}
		}

		VkResult CreateLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> deviceExtensions, std::vector<const char*> validationLayers)
		{
			float queuePriority = 1.0f;

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfoVec;

			VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
			graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			graphicsQueueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphics;
			graphicsQueueCreateInfo.queueCount = 1;
			graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfoVec.push_back(graphicsQueueCreateInfo);

			if (queueFamilyIndices.graphics != queueFamilyIndices.present)
			{
				VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
				presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				presentQueueCreateInfo.queueFamilyIndex = queueFamilyIndices.present;
				presentQueueCreateInfo.queueCount = 1;
				presentQueueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfoVec.push_back(presentQueueCreateInfo);
			}

			deviceFeatures = {};
			deviceFeatures.samplerAnisotropy = VK_TRUE;

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfoVec.size());
			createInfo.pQueueCreateInfos = queueCreateInfoVec.data();
			createInfo.pEnabledFeatures = &deviceFeatures;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

			if (validationLayers.size() > 0)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);

			if (result == VK_SUCCESS)
			{
				commandPool = CreateCommandPool(queueFamilyIndices.graphics);
			}

			return result;
		}

		VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data = nullptr)
		{
			VkResult err;

			// Create the buffer handle
			VkBufferCreateInfo bufferCreateInfo = vk::initializers::BufferCreateInfo(usageFlags, size);
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			err = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer);
			if (err != VK_SUCCESS)
			{
				return err;
			}

			// Create the memory backing up the buffer handle
			VkMemoryRequirements memReqs;
			VkMemoryAllocateInfo memAlloc = vk::initializers::MemoryAllocateInfo();
			vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			// Find a memory type index that fits the properties of the buffer
			memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
			err = vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory);
			if (err != VK_SUCCESS)
			{
				return err;
			}

			// If a pointer to the buffer data has been passed, map the buffer and copy over the data
			if (data != nullptr)
			{
				void *mapped;
				err = vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped);
				if (err != VK_SUCCESS)
				{
					return err;
				}
				memcpy(mapped, data, size);
				vkUnmapMemory(logicalDevice, *memory);
			}

			// Attach the memory to the buffer object
			err = vkBindBufferMemory(logicalDevice, *buffer, *memory, 0);
			if (err != VK_SUCCESS)
			{
				return err;
			}

			return err;
		}

		VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, Buffer* buffer, VkDeviceMemory* memory, void* data = nullptr)
		{
			VkResult err;

			// Create the buffer handle
			VkBufferCreateInfo bufferCreateInfo = vk::initializers::BufferCreateInfo(usageFlags, size);
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			err = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer);
			if (err != VK_SUCCESS)
			{
				return err;
			}

			// Create the memory backing up the buffer handle
			VkMemoryRequirements memReqs;
			VkMemoryAllocateInfo memAlloc = vk::initializers::MemoryAllocateInfo();
			vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			// Find a memory type index that fits the properties of the buffer
			memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
			err = vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory);
			if (err != VK_SUCCESS)
			{
				return err;
			}

			buffer->alignment = memReqs.alignment;
			buffer->size = memAlloc.allocationSize;
			buffer->usageFlags = usageFlags;
			buffer->memoryPropertyFlags = memoryPropertyFlags;

			// If a pointer to the buffer data has been passed, map the buffer and copy over the data
			if (data != nullptr)
			{
				err = buffer->map();
				if (err != VK_SUCCESS)
				{
					return err;
				}
				memcpy(buffer->mapped, data, size);
				buffer->unmap();
			}

			buffer->setupDescriptor();

			return buffer->bind();
		}

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue queue)
		{
			VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

			VkBufferCopy copyRegion = {};
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

			EndSingleTimeCommands(commandBuffer, queue);
		}

		void CopyBuffer(Buffer* src, Buffer* dst, VkQueue queue, VkBufferCopy* copyRegion = nullptr)
		{
			if (dst->size > src->size || !(src->buffer && dst->buffer))
			{
				throw std::runtime_error("invalid buffer copy parameters");
			}

			VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

			VkBufferCopy bufferCopy = {};
			if (copyRegion == nullptr)
			{
				bufferCopy.size = src->size;
			}
			else
			{
				bufferCopy = *copyRegion;
			}
			vkCmdCopyBuffer(commandBuffer, src->buffer, dst->buffer, 1, &bufferCopy);

			EndSingleTimeCommands(commandBuffer, queue);
		}

		bool ExtensionSupported(const char* extension)
		{
			return std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end();
		}
	};
}
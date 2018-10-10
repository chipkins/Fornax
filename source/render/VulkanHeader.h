#pragma once

#include <unordered_map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../PrecompiledHeader.h"

#include "VulkanInitializers.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"

namespace vk
{
	template <typename T>
	class ResourceList
	{
	public:
		VkDevice& device;
		std::unordered_map<std::string, T> resources;
		ResourceList(VkDevice& dev) : device(dev) {};
		const T get(std::string name)
		{
			return resources[name];
		}
		T* getPtr(std::string name)
		{
			return &resources[name];
		}
		bool doesKeyExist(std::string name)
		{
			return resources.find(name) != resources.end();
		}
	};
	class PipelineLayoutList : public ResourceList<VkPipelineLayout>
	{
	public:
		PipelineLayoutList(VkDevice& dev) : ResourceList(dev) {};
		~PipelineLayoutList()
		{
			for (auto& pipelineLayout : resources)
			{
				vkDestroyPipelineLayout(device, pipelineLayout.second, nullptr);
			}
		}
		VkPipelineLayout add(std::string name, VkPipelineLayoutCreateInfo& createInfo)
		{
			VkPipelineLayout pipelineLayout;
			if (vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create pipeline layout!");
			}
			resources[name] = pipelineLayout;
			return pipelineLayout;
		}
	};
	class PipelineList : public ResourceList<VkPipeline>
	{
	public:
		PipelineList(VkDevice& dev) : ResourceList(dev) {};
		~PipelineList()
		{
			for (auto& pipeline : resources)
			{
				vkDestroyPipeline(device, pipeline.second, nullptr);
			}
		}
		VkPipeline add(std::string name, VkGraphicsPipelineCreateInfo& pipelineCreateInfo, VkPipelineCache& pipelineCache)
		{
			VkPipeline pipeline;
			if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create graphics pipeline!");
			}
			resources[name] = pipeline;
			return pipeline;
		}
	};
	class DescriptorSetLayoutList : public ResourceList<VkDescriptorSetLayout>
	{
	public:
		DescriptorSetLayoutList(VkDevice& dev) : ResourceList(dev) {};
		~DescriptorSetLayoutList()
		{
			for (auto& descriptorSetLayout : resources)
			{
				vkDestroyDescriptorSetLayout(device, descriptorSetLayout.second, nullptr);
			}
		}
		VkDescriptorSetLayout add(std::string name, VkDescriptorSetLayoutCreateInfo createInfo)
		{
			VkDescriptorSetLayout descriptorSetLayout;
			if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor set layout");
			}
			resources[name] = descriptorSetLayout;
			return descriptorSetLayout;
		}
	};
	class DescriptorSetList : public ResourceList<VkDescriptorSet>
	{
	private:
		VkDescriptorPool descriptorPool;
	public:
		DescriptorSetList(VkDevice& dev, VkDescriptorPool pool) : ResourceList(dev), descriptorPool(pool) {};
		~DescriptorSetList()
		{
			for (auto& descriptorSet : resources)
			{
				vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet.second);
			}
		}
		VkDescriptorSet add(std::string name, VkDescriptorSetAllocateInfo allocInfo)
		{
			VkDescriptorSet descriptorSet;
			if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor set layout");
			}
			resources[name] = descriptorSet;
			return descriptorSet;
		}
	};
}
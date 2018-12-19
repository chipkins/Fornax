#pragma once

#include <chrono>

#include "VkRenderBase.h"
#include "VulkanBuffer.h"
#include "VulkanModel.h"
#include "../core/Camera.h"

class VkRenderBackend : public VkRenderBase
{
public:
	VkRenderBackend(std::vector<const char*> enabledExtensions) : VkRenderBase(enabledExtensions) {};
	virtual void Init(GLFWwindow* window);
	virtual void Cleanup();

	virtual void RequestFrameRender();

	void UpdateUniformBuffers(Camera camera, glm::vec3* deformVecs, float dt);

	//std::vector<vk::Model> GetModelList() { return m_models; }

private:

	struct Resources
	{
		vk::PipelineLayoutList* pipelineLayouts;
		vk::PipelineList *pipelines;
		vk::DescriptorSetLayoutList *descriptorSetLayouts;
		vk::DescriptorSetList * descriptorSets;
	} m_resources;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
	} vertexInput;

	struct UBOScene
	{
		glm::mat4 view;
		glm::vec3 eye;
		float fov;
		glm::vec2 resolution;
		float dt;
	} uboScene;

	struct UBOBlur
	{
		float radialBlurScale = 0.35f;
		float radialBlurStrength = 0.95f;
		glm::vec2 radialOrigin = glm::vec2(0.5f, 0.5f);
	} uboBlur;

	struct Light {
		glm::vec4 position;
		glm::vec4 color;
		float radius;
		float quadraticFalloff;
		float linearFalloff;
		float _padding;
	};

	struct UBOLight {
		Light lights[17];
		glm::vec4 eye;
		glm::mat4 view;
		glm::mat4 model;
	} uboLights;

	struct {
		vk::Buffer blur;
		vk::Buffer scene;
		vk::Buffer lights;
	} m_uniformBuffers;

	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		void Destroy(VkDevice device)
		{
			vkDestroyImage(device, image, nullptr);
			vkDestroyImageView(device, view, nullptr);
			vkFreeMemory(device, memory, nullptr);
		}
	};

	struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment depth;
		VkRenderPass renderPass;
		//VkSampler sampler;
		//VkDescriptorImageInfo descriptor;
	};

	struct {
		struct Offscreen : public FrameBuffer {
			std::array<FrameBufferAttachment, 3> attachments;
		} offscreen;
		struct Deferred : public FrameBuffer {
			std::array<FrameBufferAttachment, 1> attachments;
		} lights;
	} m_framebuffers;

	VkSampler      m_colorSampler;

	VkCommandBuffer m_offscreenCommandBuffer = VK_NULL_HANDLE;
	VkSemaphore     m_offscreenSemaphore = VK_NULL_HANDLE;

	// Vulkan Initialization Functions
	void CreateDescriptorSetLayout();
	/*void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();*/

	// Create a frame buffer attachment
	void CreateAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment, uint32_t width, uint32_t height);
	void PrepareOffscreenFramebuffers();
	void BuildDeferredCommandBuffer();
	virtual void BuildCommandBuffers();
	//void RebuildCommandBuffers();
	void SetupDescriptorPool();
	void SetupLayoutsAndDescriptors();
	void PreparePipelines();
	void PrepareUniformBuffers();
	void SetupLights();

	void Draw();

	// Helper Functions
	void SetupLight(Light *light, glm::vec3 pos, glm::vec3 color, float radius);
};
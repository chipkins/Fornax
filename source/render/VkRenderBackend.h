#pragma once

#include "VulkanHeader.h"
#include "VkRenderBase.h"
#include "VulkanBuffer.h"
#include "VulkanModel.h"
#include "../core/Camera.h"

class VkRenderBackend : public VkRenderBase
{
public:
	virtual void Init(GLFWwindow* window);
	virtual void Cleanup();

	virtual void RequestFrameRender();

	void UpdateUniformBuffers(Camera camera, glm::vec3* deformVecs, float dt);

	std::vector<vk::Model> GetModelList() { return m_models; }

private:

	struct UBOScene
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		//glm::vec3 deformVec[121];
	} uboScene;

	struct UBOBlur
	{
		float radialBlurScale = 0.35f;
		float radialBlurStrength = 0.75;
		glm::vec2 radialOrigin = glm::vec2(0.5f, 0.5f);
	} uboBlur;

	struct {
		vk::Buffer blur;
		vk::Buffer scene;
	} m_uniformBuffers;

	struct {
		VkPipeline blur;
		VkPipeline scene;
	} m_pipelines;

	struct {
		VkPipelineLayout blur;
		VkPipelineLayout scene;
	} m_pipelineLayouts;

	struct {
		VkDescriptorSet blur;
		VkDescriptorSet scene;
	} m_descriptorSets;

	struct {
		VkDescriptorSetLayout blur;
		VkDescriptorSetLayout scene;
	} m_descriptorSetLayouts;

	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
	};

	struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment color, depth;
		VkRenderPass renderPass;
		VkSampler sampler;
		VkDescriptorImageInfo descriptor;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkSemaphore semaphore = VK_NULL_HANDLE;
	} m_offScreenFrameBuffer;

	VkImage        m_textureImage;
	VkImageView    m_textureImageView;
	VkDeviceMemory m_textureImageMemory;
	VkSampler      m_textureSampler;
	
	std::vector<vk::Model> m_models;
	VkBuffer               m_vertexBuffer;
	VkDeviceMemory         m_vertexBufferMemory;
	VkBuffer               m_indexBuffer;
	VkDeviceMemory         m_indexBufferMemory;

	// Vulkan Initialization Functions
	void CreateDescriptorSetLayout();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	// Create a frame buffer attachment
	//void CreateAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment, uint32_t width, uint32_t height);
	void PrepareOffscreenFramebuffer();
	void BuildOffscreenCommandBuffer();
	virtual void BuildCommandBuffers();
	//void RebuildCommandBuffers();
	void SetupDescriptorPool();
	void SetupLayoutsAndDescriptors();
	void PreparePipelines();
	void PrepareUniformBuffers();
	//void SetupLights();

	void Draw();
	void Prepare();

	// Helper Functions
};
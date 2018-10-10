#pragma once

#include "VulkanHeader.h"
#include "Model.h"
#include "Camera.h"

const std::vector<const char*> c_validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> c_deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class VkRenderBackend 
{
public:
	void Init(GLFWwindow* window);
	void Cleanup();

	void RequestFrameRender();

	void UpdateUniformBuffer(Camera camera, glm::vec3* deformVecs, float dt);

	void WaitForDrawFinish();

	void RecreateSwapchain();

	std::vector<Model> GetModelList() { return m_models; }

private:

	struct UBOScene
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 deformVec[121];
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
	
	VkDebugReportCallbackEXT m_callback;

	std::vector<Model> m_models;

	// Vulkan Initialization Functions
	void SetupDebugCallback();
	void SelectPhysicalDevice();
	void CreateLogicalDeviceAndQueues();
	void CreateImageViews();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateDepthResources();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffer();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateSephamores();

	void CleanupSwapchain();

	// Helper Functions
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);

	VkCommandBuffer BeginSingleTimeCommands();
	void            EndSingleTimeCommands(VkCommandBuffer commandBuffer);
};
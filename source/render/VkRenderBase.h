#pragma once

#include "VulkanHeader.h"
#include "VulkanInitializers.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanWindow.h"

#ifdef NDEBUG
const bool c_enableValidationLayers = false;
#else
const bool c_enableValidationLayers = true;
#endif

class VkRenderBase
{
protected:
	VkInstance m_instance;

	struct VkContext
	{
		VkQueue               graphicsQueue;
		VkQueue               presentQueue;
		VkFormat              depthFormat;
		VkRenderPass          renderPass;
		VkPipelineCache       pipelineCache;
		VkSampleCountFlagBits sampleCount;
		bool                  supersampling;
	} m_context;

	vk::Device* m_device;
	vk::Window* m_window;

	vk::Swapchain              m_swapchain;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	VkCommandPool                m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	VkPipelineCache m_pipelineCache;

	struct Image {
		VkImage        image;
		VkDeviceMemory memory;
		VkImageView    view;
	} m_depthStencil;

	struct {
		VkSemaphore imageAvailable;
		VkSemaphore renderFinished;
	} m_semaphores;

	//std::vector<VkFramebuffer> m_framebuffers;
	uint32_t                   m_currentBuffer;

	VkDescriptorPool            m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkShaderModule> m_shaderModules;

	VkSubmitInfo         m_submitInfo;
	VkPipelineStageFlags m_submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkDebugReportCallbackEXT m_callback;

	VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };

	std::vector<const char*> m_validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	std::vector<const char*> m_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

private:
	void CreateInstance();
	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayerSupport();

public:
	VkRenderBase() {};
	VkRenderBase(std::vector<const char*> enabledExtensions);
	void CreateGLFWSurface(GLFWwindow* window);
	void SelectPhysicalDevice();
	void CreateLogicalDeviceAndQueues();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreatePipelineCache();
	void CreateSemaphores();

	void WaitForDrawToFinish();
	void CleanupSwapchain();
	VkPipelineShaderStageCreateInfo LoadShader(std::string, VkShaderStageFlagBits stage);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();

	void PrepareFrame();
	void SubmitFrame();

	// Virtual functions that can be overriden
	//virtual ~VkRenderBase();
	virtual void Cleanup();
	virtual void Init(GLFWwindow* window);
	virtual void SetupDepthStencil();
	virtual void SetupSwapchain();
	virtual void SetupFrameBuffers();
	virtual void SetupRenderPass();
	virtual void SetupDebugCallback();
	virtual void ResizeWindow();

	// Pure virtual functions
	virtual void RequestFrameRender() = 0;
	virtual void BuildCommandBuffers() = 0;
};
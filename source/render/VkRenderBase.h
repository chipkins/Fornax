#pragma once

#include "VulkanHeader.h"
#include "VulkanInitializers.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"

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
		vk::Device            device;
		VkQueue               graphicsQueue;
		VkQueue               presentQueue;
		VkFormat              depthFormat;
		VkRenderPass          renderPass;
		VkPipelineCache       pipelineCache;
		VkSampleCountFlagBits sampleCount;
		bool                  supersampling;
	} m_context;

	struct VkWindow {
		GLFWwindow*  windowPtr;
		VkSurfaceKHR surface;
		int32_t width, height;
	} m_window;

	vk::Swapchain              m_swapchain;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	VkCommandPool                m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	VkPipelineCache m_pipelineCache;

	struct Image {
		VkImage        image;
		VkDeviceMemory memory;
		VkImageView    view;
	} depthStencil;

	struct {
		VkSemaphore imageAvailable;
		VkSemaphore renderFinished;
	} m_semaphores;

	std::vector<VkFramebuffer> m_framebuffers;
	uint32_t                   m_currentBuffer;

	VkDescriptorPool            m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkShaderModule> m_shaderModules;

	std::vector<const char*> m_validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	std::vector<const char*> m_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

private:
	VkResult CreateInstance();
	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayerSupport();

public:
	VkRenderBase();
	void Init(GLFWwindow* window);
	void CreateGLFWSurface();
	VkPhysicalDevice SelectPhysicalDevice();
	void CreateSwapchain();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreatePipelineCache();

	void DestroyCommandBuffers();

	void ResizeWindow();
	void WaitForDrawToFinish();
	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin);
	VkPipelineShaderStageCreateInfo LoadShader(const char* filename, VkShaderStageFlagBits stage);

	void PrepareFrame();
	void SubmitFrame();

	// Virtual functions that can be overriden
	virtual ~VkRenderBase();
	virtual void Cleanup();
	virtual void SetupDepthStencil();
	virtual void SetupSwapchain();
	virtual void SetupFrameBuffers();
	virtual void SetupRenderPass();

	// Pure virtual functions
	virtual void RequestFrameRender() = 0;
	virtual void BuildCommandBuffers() = 0;
};
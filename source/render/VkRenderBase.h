#pragma once

#include "VulkanHeader.h"

#ifdef NDEBUG
const bool c_enableValidationLayers = false;
#else
const bool c_enableValidationLayers = true;
#endif

namespace vk
{
	class VkRenderBase
	{
	protected:
		VkInstance m_instance;

		struct VkContext
		{
			Device                device;
			int32_t               graphicsFamilyIndex;
			int32_t               presentFamilyIndex;
			VkQueue               graphicsQueue;
			VkQueue               presentQueue;
			VkFormat              depthFormat;
			VkRenderPass          renderPass;
			VkPipelineCache       pipelineCache;
			VkSampleCountFlagBits sampleCount;
			bool                  supersampling;
		} m_context;

		struct VkWindow
		{
			GLFWwindow*  windowPtr;
			VkSurfaceKHR surface;
			int32_t width, height;
		} m_window;

		VkSwapchainKHR             m_swapchain;
		std::vector<VkImage>       m_swapchainImages;
		std::vector<VkImageView>   m_swapchainImageViews;
		VkFormat                   m_swapchainImageFormat;
		VkExtent2D                 m_swapchainExtent;
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

	private:
		VkResult CreateInstance();
		std::vector<const char*> GetRequiredExtensions();

	public:
		VkRenderBase();
		void Init(GLFWwindow* window);
		bool InitVulkan();
		void CreateSurface();
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
}
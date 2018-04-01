#pragma once

#include "../PrecompiledHeader.h"
#include "Model.h"

#ifdef NDEBUG
	const bool c_enableValidationLayers = false;
#else
	const bool c_enableValidationLayers = true;
#endif

const std::vector<const char*> c_validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> c_deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct GPUInfo
{
	VkPhysicalDevice                     physicalDevice;
	VkPhysicalDeviceProperties           deviceProperties;
	VkPhysicalDeviceFeatures             deviceFeatures;
	VkPhysicalDeviceMemoryProperties     memoryProperties;
	VkSurfaceCapabilitiesKHR             surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR>      surfaceFormats;
	std::vector<VkPresentModeKHR>        presentModes;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	std::vector<VkExtensionProperties>   extensionProperties;
};

struct VkContext
{
	VkDevice              device;
	GPUInfo               gpu;
	int32_t               graphicsFamilyIndex;
	int32_t               presentFamilyIndex;
	VkQueue               graphicsQueue;
	VkQueue               presentQueue;
	VkFormat              depthFormat;
	VkRenderPass          renderPass;
	VkPipelineCache       pipelineCache;
	VkSampleCountFlagBits sampleCount;
	bool                  supersampling;
};

struct VkWindow
{
	GLFWwindow*  windowPtr;
	VkSurfaceKHR surface;
	int32_t width, height;
};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class VkRenderBackend {
public:
	void Init(GLFWwindow* window);
	void Cleanup();

	void RequestFrameRender();

	void UpdateUniformBuffer();

	void WaitForDrawFinish();

	void RecreateSwapchain();

private:
	VkInstance m_instance;
	VkContext  m_context;
	VkWindow   m_window;
	
	VkSwapchainKHR             m_swapchain;
	std::vector<VkImage>       m_swapchainImages;
	std::vector<VkImageView>   m_swapchainImageViews;
	VkFormat                   m_swapchainImageFormat;
	VkExtent2D                 m_swapchainExtent;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	VkPipelineLayout      m_pipelineLayout;
	VkPipeline            m_graphicsPipeline;

	VkCommandPool                m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	VkBuffer       m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer       m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
	VkBuffer       m_uniformBuffer;
	VkDeviceMemory m_uniformBufferMemory;

	VkDescriptorSetLayout m_descriptorSetLayout;
	VkDescriptorPool      m_descriptorPool;
	VkDescriptorSet       m_descriptorSet;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	
	VkDebugReportCallbackEXT m_callback;

	// Vulkan Initialization Functions
	void CreateInstance();
	void SetupDebugCallback();
	void CreateSurface();
	void SelectPhysicalDevice();
	void CreateLogicalDeviceAndQueues();
	void CreateSwapchain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffer();
	void CreateDescriptorPool();
	void CreateDescriptorSet();
	void CreateCommandBuffers();
	void CreateSephamores();

	void CleanupSwapchain();

	// Helper Functions
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();

	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	void FindQueueFamilies(VkPhysicalDevice device);

	void QuerySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
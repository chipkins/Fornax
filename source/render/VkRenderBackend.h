#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

class VkRenderBackend {
public:
	void Init(GLFWwindow* window);
	void Cleanup();

	void RequestFrameRender();

private:
	VkInstance m_instance;
	VkContext  m_context;

	VkSurfaceKHR                         m_surface;
	std::vector<VkSurfaceFormatKHR>      m_surfaceFormats;
	std::vector<VkPresentModeKHR>        m_presentModes;
	
	VkSwapchainKHR           m_swapchain;
	std::vector<VkImage>     m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkFormat                 m_swapchainImageFormat;
	VkExtent2D               m_swapchainExtent;
	
	VkDebugReportCallbackEXT m_callback;

	// Vulkan Initialization Functions
	void CreateInstance();
	void SetupDebugCallback();
	void CreateSurface(GLFWwindow* window);
	void SelectPhysicalDevice();
	void CreateLogicalDeviceAndQueues();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateGraphicsPipeline();

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
};
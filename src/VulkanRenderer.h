#pragma once

//if we weren't using GLFW, we'd have to tell vulkan
//which platform to use surfaces for. e.g. for windows:
//#define VK_USE_PLATFORM_WIN32_KHR
//this will then enable us to use VkWin32SurfaceCreateInfoKHR
//
//however, since we use GLFW, we can make this agnostic:
//glfwCreateWindowSurface -> create VkSurfaceKHR 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>

#include "Utilities.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	int32_t Init(GLFWwindow* newWindow);

	void Draw();

	void CleanUp();

private:
	//vulkan functions
	// - vk create functions
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();
	void CreateSwapChain();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronizationObjects();

	// -  record functions
	void RecordCommands();

	// - vk getter functions
	void GetPhysicalDevice();

	// - vk support functions
	//	 - vk support checker functions
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	bool CheckPhysicalDeviceSuitable(const VkPhysicalDevice& device);

	//	 - vk support getter functions
	QueueFamilyIndicies GetQueueFamilies(VkPhysicalDevice physicalDevice);
	SwapChainDetails GetSwapChainDetails(VkPhysicalDevice physicalDevice);

	// - support functions for choosing best options
	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& modes);
	VkExtent2D ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);

	// - support create functions
	VkImageView CreateImageView(const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char> &code);

	//adding required extensions
	std::vector<const char*> GetRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* pUserData);

	//validation layers
	//first create the creation info struct (separately, so that it can be used
	//	for both the DebugMessenger we use for general debugging, as well as the
	//	extra debug messenger for the creation of the instance, which has to be
	//	handled separately)
	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void SetupDebugMessenger();
	bool CheckValidationLayerSupport();

	//proxy functions to create and destroy debug utils messenger by looking up the address
	//and then calling it
	static VkResult CreateDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

private:
	GLFWwindow* Window = nullptr;

	uint32_t CurrentFrame = 0;

	// vulkan components
	// - Main
	VkInstance Instance;
	struct {
		VkPhysicalDevice PhysicalDevice;
		VkDevice LogicalDevice;
	} MainDevice;

	//these "queues" are just handles to the actual data, they don't contain the data themselves
	VkQueue GraphicsQueue;
	VkQueue PresentationQueue;

	//surface that we render to with vulkan.
	//GLFW will take this surface and present it to the viewer
	VkSurfaceKHR Surface;

	VkSwapchainKHR Swapchain;
	//the following three are 1:1 connected. One framebuffer per image, one commandbuffer per framebuffer
	std::vector<SwapchainImage> SwapchainImages;
	std::vector<VkFramebuffer> SwapchainFramebuffers;		//one framebuffer per swapchain image
	std::vector<VkCommandBuffer> CommandBuffers;

	// - Pipeline
	VkPipeline GraphicsPipeline;
	VkPipelineLayout PipelineLayout;
	VkRenderPass RenderPass;

	// - Pools
	VkCommandPool GraphicsCommandPool;

	// - Utility
	VkFormat SwapchainImageFormat;
	VkExtent2D SwapchainResolution;

	// - Synchronisation
	std::vector<VkSemaphore> ImagesAvailable;
	std::vector<VkSemaphore> RendersFinished;
	std::vector<VkFence> DrawFences;

	//validation layers
	VkDebugUtilsMessengerEXT DebugMessenger;
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

#ifdef NDEBUG
	const bool bEnableValidationLayers = false;
#else
	const bool bEnableValidationLayers = true;
#endif
};


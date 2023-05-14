#include "VulkanRenderer.h"

#include <cstring>
#include <iostream>

VulkanRenderer::VulkanRenderer()
{

}

VulkanRenderer::~VulkanRenderer()
{

}

int32_t VulkanRenderer::Init(GLFWwindow* newWindow)
{
	Window = newWindow;

	try
	{
		CreateInstance();

		//surface is not create for any particular device, but for an instance
		//thus create the surface first and make sure the device supports it
		CreateSurface();		
		GetPhysicalDevice();
		CreateLogicalDevice();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}
	

	return 0;
}

void VulkanRenderer::CleanUp()
{
	vkDestroyInstance(Instance, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	vkDestroyDevice(MainDevice.LogicalDevice, nullptr);
}

void VulkanRenderer::CreateInstance()
{
	//enable validation layers
	if(bEnableValidationLayers && ! CheckValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available");
	}
	else if (bEnableValidationLayers)
	{
		std::cout << "Validation layers are enabled!" << std::endl;
	}

	//information about the application itself
	//most data here doesn't affect the program and is for developer convenience
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Tutorial App";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "No Engine";
	//the previous info is just for the developer
	//the apiInfo relates to the Vulkan API Version though, this can affect the program!
	appInfo.apiVersion = VK_API_VERSION_1_0;

	//creation information for a VkInstance
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	//create list to hold instance extensions
	std::vector<const char*> instanceExtensions = GetRequiredExtensions();

	//check instance extensions are supported
	if (!CheckInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	//TODO: setting up validation layers
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;	

	//create the actual VkInstance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &Instance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan instance!");
	}
}

void VulkanRenderer::CreateLogicalDevice()
{
	//get the queue family indices for the chosen physical device
	QueueFamilyIndicies indices = GetQueueFamilies(MainDevice.PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	//use a set for this to avoid accessing the same index twice
	std::set<int32_t> queueFamilyIndices = { indices.GraphicsFamily, indices.PresentationFamily};

	for (int32_t queueFamilyIndex : queueFamilyIndices)
	{
		//queues the logical device needs to create
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;		//1 is highest priority

		queueCreateInfos.push_back(queueCreateInfo);
	}

	//physical device features that the logical device will be using (e.g. geometry shader)
	VkPhysicalDeviceFeatures features = {};

	//information to create logical device (sometimes only called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	//note that there is a difference between VkInstance Extensions and Vk(Logical)Device Extensions!
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = &features;

	//create logical device for the given physical device
	//this implicitly also creates the queues which we can then fetch later with vkGetDeviceQueue
	VkResult result = vkCreateDevice(MainDevice.PhysicalDevice, &deviceCreateInfo, nullptr, &MainDevice.LogicalDevice);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a logical device!");
	}

	//VkCreateDevice also implicitly created the queue(s)
	//we still need to get a handle for these
	vkGetDeviceQueue(MainDevice.LogicalDevice, indices.GraphicsFamily, 0, &GraphicsQueue);
	vkGetDeviceQueue(MainDevice.LogicalDevice, indices.PresentationFamily, 0, &PresentationQueue);
}

void VulkanRenderer::CreateSurface()
{
	//create surface:
	//	glfw is creating a surface create info struct,
	//	then runs the create surface function and returns result
	VkResult result = glfwCreateWindowSurface(Instance, Window, nullptr, &Surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a surface!");
	}
}

void VulkanRenderer::GetPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		//if no devices are available, then none support vulkan!
		throw std::runtime_error("Can't find GPUs that support Vulkan Instance!");
	}

	std::vector<VkPhysicalDevice> physicalDevicesList(deviceCount);
	vkEnumeratePhysicalDevices(Instance, &deviceCount, physicalDevicesList.data());

	for (const auto& device : physicalDevicesList)
	{
		if (CheckPhysicalDeviceSuitable(device))
		{
			MainDevice.PhysicalDevice = device;
			break;
		}
	}
}

bool VulkanRenderer::CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	//check how many extensions are supported in total
	uint32_t totalExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &totalExtensionCount, nullptr);

	//now create a list of extensions with the correct amount of entries, such that it can be populated by vulkan
	std::vector<VkExtensionProperties> extensions(totalExtensionCount);

	//now have vulkan populate the list
	vkEnumerateInstanceExtensionProperties(nullptr, &totalExtensionCount, extensions.data());

	//check if given extensions are in list of available extensions
	for (const auto& checkExtension : *checkExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName))
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	// get device extension count
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	// if no extensions were found, return failure
	if (extensionCount == 0)
	{
		return false;
	}

	// populate list of extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	// check for extension
	for (const auto& deviceExtension : DeviceExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::CheckPhysicalDeviceSuitable(const VkPhysicalDevice& device)
{
	//information about the device itself (ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	//information about what the device can do (e.g. geometry shader, tessellation shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	//TODO: actually handle on device properties and features being present or missing

	//check that our wanted queue(s) are supported
	QueueFamilyIndicies indices = GetQueueFamilies(device);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainValid = false;

	if (extensionsSupported)
	{
		SwapChainDetails swapChainDetails = GetSwapChainDetails(device);
		swapChainValid = !swapChainDetails.PresentationModes.empty() && !swapChainDetails.SurfaceFormats.empty();
	}

	//device is only suitable, if it has all the queue families
	//and has all the extensions
	//and the swapchain capabilities include image formats and presentaton modes
	return indices.IsValid() && extensionsSupported && swapChainValid;
}

QueueFamilyIndicies VulkanRenderer::GetQueueFamilies(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndicies indices;

	uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyList.data());

	//go through each queue family and check if it has at least one of the required types of queue
	int32_t i = 0;
	for (const auto& queueFamily : queueFamilyList)
	{
		// first check if the queue family has at least 1 queue in that family (could have no queues)
		// queue can be multiple types defined through bitfield -> find the correct one via bit comparison
		if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.GraphicsFamily = i;
		}

		// Check if Queue Family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, Surface, &presentationSupport);

		// Check if Queue is presentation type (can be both graphics and presentation)
		if (queueFamily.queueCount > 0 && presentationSupport)
		{
			indices.PresentationFamily = i;
		}

		if (indices.IsValid())
		{
			//all required queues are found and their indices stored
			//no need to continue looping through further queue families
			break;
		}

		i++;
	}

	return indices;
}

SwapChainDetails VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice physicalDevice)
{
	SwapChainDetails details;

	// CAPABILITIES
	// get the surface capabilities for the given surface on the given physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, Surface, &details.SurfaceCapabilities);

	// FORMAT
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, Surface, &formatCount, nullptr);

	// if formats returned, get list of formats
	if (formatCount != 0)
	{
		details.SurfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, Surface, &formatCount, details.SurfaceFormats.data());
	}

	// PRESENTATION MODES
	uint32_t presentationCount = 0;

	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, Surface, &presentationCount, nullptr);

	if (presentationCount != 0)
	{
		details.PresentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, Surface, &presentationCount, details.PresentationModes.data());

	}

	return details;
}

std::vector<const char*> VulkanRenderer::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if(bEnableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanRenderer::CheckValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for(const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
				if(strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
		}

		if(!layerFound)
		{
			return false;
		}
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* pUserData)
{
	std::cerr << "validation layer: " << callbackData->pMessage << std::endl;

	return VK_FALSE;
}

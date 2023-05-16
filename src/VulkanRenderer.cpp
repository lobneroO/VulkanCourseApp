#include "VulkanRenderer.h"

#include <cstring>
#include <iostream>
#include <limits>
#include <algorithm>
#include <array>

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

		//enable validation layer output
		SetupDebugMessenger();

		//surface is not create for any particular device, but for an instance
		//thus create the surface first and make sure the device supports it
		CreateSurface();		
		GetPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateRenderPass();
		CreateGraphicsPipeline();
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
	vkDestroyPipeline(MainDevice.LogicalDevice, GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(MainDevice.LogicalDevice, PipelineLayout, nullptr);
	vkDestroyRenderPass(MainDevice.LogicalDevice, RenderPass, nullptr);
	for(SwapchainImage image : SwapchainImages)
	{
		vkDestroyImageView(MainDevice.LogicalDevice, image.ImageView, nullptr);
	}
	vkDestroySwapchainKHR(MainDevice.LogicalDevice, Swapchain, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	vkDestroyDevice(MainDevice.LogicalDevice, nullptr);
	if(bEnableValidationLayers)
	{
		DestroyDebugUtilsMessenger(Instance, DebugMessenger, nullptr);
	}
	vkDestroyInstance(Instance, nullptr);
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

	//if no validation layers:
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.pNext = nullptr;

	//enable validation layers for the actual instance creation
	//	(i.e. this is separate from the rest of the validation layer cheks with the DebugMessenger)
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if(bEnableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		VulkanRenderer::PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}

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

void VulkanRenderer::CreateSwapChain()
{
	// get swapchain details so we can pick best settings
	SwapChainDetails swapChainDetails = GetSwapChainDetails(MainDevice.PhysicalDevice);

	// find optimal values
	VkSurfaceFormatKHR format = ChooseBestSurfaceFormat(swapChainDetails.SurfaceFormats);
	VkPresentModeKHR mode = ChooseBestPresentationMode(swapChainDetails.PresentationModes);
	VkExtent2D resolution = ChooseSwapChainExtent(swapChainDetails.SurfaceCapabilities);

	// get num images in swap chain. get 1 more than min to allow triple buffering
	uint32_t imageCount = swapChainDetails.SurfaceCapabilities.minImageCount + 1;

	if(swapChainDetails.SurfaceCapabilities.maxImageCount > 0
		&& swapChainDetails.SurfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.SurfaceCapabilities.maxImageCount;
	}

	//create swapchain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = Surface;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.presentMode = mode;
	createInfo.imageExtent = resolution;
	createInfo.minImageCount = imageCount;
	//number of layers for each image in chain
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = swapChainDetails.SurfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	//whether to clip parts of image not in view (e.g. behind other window, off screen, ...)
	createInfo.clipped = VK_TRUE;

	// get queue family indices
	QueueFamilyIndicies indices = GetQueueFamilies(MainDevice.PhysicalDevice);

	// if graphics and presentation families are different, then swapchain must
	// images be shared between families
	if(indices.GraphicsFamily != indices.PresentationFamily)
	{
		//queues to share between
		uint32_t queueFamilyIndices[] = {
			(uint32_t) indices.GraphicsFamily,
			(uint32_t) indices.PresentationFamily
		};

		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		//the next two don't have to be specified for this case,
		//just doing it for clariy
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	// we could take over work/responsibilities from an older swap chain,
	// useful e.g. when resizing the window, which means
	// to destory the old swapchain and creatign a new one
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(MainDevice.LogicalDevice, &createInfo, nullptr, &Swapchain);
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a swapchain!");
	}

	//store for later reference
	SwapchainImageFormat = format.format;
	SwapchainResolution = resolution;

	// get swapchain images
	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(MainDevice.LogicalDevice, Swapchain, &swapchainImageCount, nullptr);

	std::vector<VkImage> images(swapchainImageCount);
	vkGetSwapchainImagesKHR(MainDevice.LogicalDevice, Swapchain, &swapchainImageCount, images.data());

	for(VkImage image : images)
	{
		//store image handle (note that VkImage is just uint64_t, i.e. an index and we can just copy the value
		SwapchainImage scImage = {};
		scImage.Image = image;
		scImage.ImageView = CreateImageView(image, SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		SwapchainImages.push_back(scImage);
	}
}

void VulkanRenderer::CreateRenderPass()
{
	// colour attachment of render pass
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.format = SwapchainImageFormat;
	// same as creating in the rasterizer mutlisampling
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// what to do with the attachment before rendering
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// what to do with the attachment after rendering
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp =  VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp =  VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operations

	// layout before render pass starts (the expected one, not the one to change to
	//										--> conversion happens in subpass)
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// layout after render pass (the one to change to)
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// attachment reference uses an attachment index
	// that refers to index in the attachment list passed to renderPassCreateInfo
	// (can be reused for other passes)
	VkAttachmentReference colourAttachmentRef = {};
	// index in the list
	colourAttachmentRef.attachment = 0;
	// the layout the colourAttachment.initialLayout is converted to
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// information about particular subpass the renderpass is using
	VkSubpassDescription subpass = {};
	// you can choose ray tracing here
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentRef;

	// need to determine when layout transitions occurr using subpass dependencies
	std::array<VkSubpassDependency, 2> subpassDependencies;

	// conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	//			--- transition must happen after
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;	//SUBPASS_EXTERNAL: special value meaning outside of render pass
	// what stage in the pipeline has to happen before this can take place
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	//			--- transition must happen before
	subpassDependencies[0].dstSubpass = 0;	//id of subpass
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colourAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(MainDevice.LogicalDevice, &renderPassCreateInfo, nullptr, &RenderPass);
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a render pass!");
	}
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	auto vertexShaderCode = ReadShaderFile(GetShaderPath() / fs::path("vert.spv"));
	auto fragmentShaderCode = ReadShaderFile(GetShaderPath() / fs::path("frag.spv"));

	// build shader modules to link to graphics CreateGraphicsPipeline
	// we don't need these after creating the pipeline, they can be destroyed at the end of this function
	VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

	// -- Shader stage creation information
	// vertex stage creation info
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	{
		vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderCreateInfo.module = vertexShaderModule;
		// function name for the function run first in the shader
		// in OpenGL / glsl, the default is main, so keep with that
		vertexShaderCreateInfo.pName = "main";
	}

	// fragment stage  creation info
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	{
		fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderCreateInfo.module = fragmentShaderModule;
		fragmentShaderCreateInfo.pName = "main";
	}

	// put shader stage creation infos into array
	// graphics pipeline creation info requires array of shader stage creates
	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertexShaderCreateInfo, fragmentShaderCreateInfo
	};

	// -- Vertex input (TODO: add vertex descriptions when resources created)
	VkPipelineVertexInputStateCreateInfo vertInputCreateInfo = {};
	{
		vertInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// list of vertex binding descriptions (data spacing / stride)
		vertInputCreateInfo.vertexBindingDescriptionCount = 0;
		vertInputCreateInfo.pVertexBindingDescriptions = nullptr;
		// list of vertex attribute descriptions (data format and where to bind to / from)
		vertInputCreateInfo.vertexAttributeDescriptionCount = 0;
		vertInputCreateInfo.pVertexAttributeDescriptions = nullptr;
	}

	// -- Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	{
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// restart primitive is used for strips (rather than lists), to start a new strip at some point
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
	}

	// -- Viewport and scissor
	//	create a viewport info struct
	VkViewport viewport = {};
	{
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)SwapchainResolution.width;
		viewport.height = (float)SwapchainResolution.height;
		// framebuffer depth
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
	}

	// create a scissor info struct
	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = SwapchainResolution;

	VkPipelineViewportStateCreateInfo vpStateCreateInfo = {};
	{
		vpStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vpStateCreateInfo.viewportCount = 1;
		vpStateCreateInfo.pViewports = &viewport;
		vpStateCreateInfo.scissorCount = 1;
		vpStateCreateInfo.pScissors = &scissor;
	}

	// -- Dynamic states (not used here. if used, e.g. for resizing window, you still have to recreate the swapchain with new size!
	{
		// dynamic states to enable
		std::vector<VkDynamicState> dynamicStateEnables;
		// dynamic vp - resize in command buffer with vkCmdSetViewport(cmdBuffer, viewportIndex, numViewports, array of viewports)
		// 		e.g. vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		// same as vp: vkCmdSetScissor
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

		// dynamic state creation info
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
	}

	// - Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	{
		rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// whether to clip past far plane (TRUE, default) or clip into farplane (FALSE)
		rasterizerCreateInfo.depthClampEnable = VK_FALSE;
		// whether to discard data and skip rasterizer (never creates fragments)
		rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCreateInfo.lineWidth = 1.0f;		//values != 1 need extension
		rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	}

	// -- Multisampling (disabled for now)
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	{
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		// number of samples to use per fragment (not necessary when disabled)
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	}

	// -- Blending
	// blend attachment state
	VkPipelineColorBlendAttachmentState colourBlendState = {};
	{
		// channels on which to blend
		colourBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colourBlendState.blendEnable = VK_TRUE;

		// blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
		colourBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colourBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colourBlendState.colorBlendOp = VK_BLEND_OP_ADD;

		//summarised: VK_BLEND_FACTOR_SRC_ALPHA * new colour + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
		//				(new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

		colourBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colourBlendState.alphaBlendOp = VK_BLEND_OP_ADD;

		//summarised: (1 * new alpha) + (0 * old alpha) = new alpha
	}

	VkPipelineColorBlendStateCreateInfo colourBlendCreateInfo = {};
	{
		colourBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		// alternative to calculations is to use logical oerpations
		colourBlendCreateInfo.logicOpEnable = VK_FALSE;
		colourBlendCreateInfo.attachmentCount = 1;
		colourBlendCreateInfo.pAttachments = &colourBlendState;
	}

	// -- Pipeline layout (TODO: apply future descriptor set layouts)
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	{
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 0;
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	}

	// create pipeline layout
	VkResult result = vkCreatePipelineLayout(MainDevice.LogicalDevice, &pipelineLayoutCreateInfo, nullptr, &PipelineLayout);
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	// -- Depth stencil testing
	//TODO: setup depth stencil testing

	// -- Graphics pipeline creation
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	{
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;	//number of shader stages
		pipelineCreateInfo.pStages = shaderStages;
		pipelineCreateInfo.pVertexInputState = &vertInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineCreateInfo.pViewportState = &vpStateCreateInfo;
		pipelineCreateInfo.pDynamicState = nullptr;
		pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colourBlendCreateInfo;
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.layout = PipelineLayout;
		pipelineCreateInfo.renderPass = RenderPass;
		pipelineCreateInfo.subpass = 0;		//index of subpass of render pass to use with pipeline

		// pipeline derivatives
		// for shared settings usage
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;		//existing pipeline to derive from
		pipelineCreateInfo.basePipelineIndex = -1;					//or index of pipeline being created to derive from
	}

	result = vkCreateGraphicsPipelines(MainDevice.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &GraphicsPipeline);
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	// destroy shader modules, as they are no longer needed after pipeline creation
	vkDestroyShaderModule(MainDevice.LogicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(MainDevice.LogicalDevice, vertexShaderModule, nullptr);
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

VkSurfaceFormatKHR VulkanRenderer::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	// best format is subjective, but ours will be:
	//	Format:		VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
	//	colorspace:	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

	if(formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		//this will mean: all formats are supported
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for(const auto& format : formats)
	{
		if((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
			&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& modes)
{
	for(const auto& presentationMode : modes)
	{
		if(presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	// according to the vulkan spec, this always has to be available.
	// therefore it can be used as a back up
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities)
{
	if(surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		int32_t width, height;
		glfwGetFramebufferSize(Window, &width, &height);

		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		// make sure the extent is within max and min of the surface
		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
								   std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
									std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}

VkImageView VulkanRenderer::CreateImageView(const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspectFlags)
{
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	//setup swizzle if desired (identity -> no remapping)
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// subresources allow the view ti view only a part of an image
	createInfo.subresourceRange.aspectMask = aspectFlags;
	//start mip map level to view from
	createInfo.subresourceRange.baseMipLevel = 0;
	//number of mip map level to view
	createInfo.subresourceRange.levelCount = 1;
	//start array layer to view from
	createInfo.subresourceRange.baseArrayLayer = 0;
	//number of array layers
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VkResult result = vkCreateImageView(MainDevice.LogicalDevice, &createInfo, nullptr, &imageView);
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create an image view!");
	}

	return imageView;
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char> &code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(MainDevice.LogicalDevice, &createInfo, nullptr, &shaderModule);
	if(result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
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

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* pUserData)
{
	if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cerr << "validation layer: " << callbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

void VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VulkanRenderer::DebugCallback;
		createInfo.pUserData = nullptr;
}

void VulkanRenderer::SetupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if(CreateDebugUtilsMessenger(Instance, &createInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to setup debug messenger!");
	}
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

VkResult VulkanRenderer::CreateDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if(func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanRenderer::DestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if(func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

#pragma once

#include <cstdint>

const std::vector<const char*> DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//indices of the locations of queue families (if they exist at all)
struct QueueFamilyIndicies
{
	int32_t GraphicsFamily = -1;		//Location of Graphics Queue Family
	int32_t PresentationFamily = -1;	//Location of Presentation Queue Family (can be same as Graphics Family)
	
	/**
	* check if queue families are valid
	*/
	bool IsValid()
	{
		return GraphicsFamily >= 0 && PresentationFamily >= 0;
	}
};

struct SwapChainDetails
{
	//surface properties, e.g. image size
	VkSurfaceCapabilitiesKHR SurfaceCapabilities;

	//list of supported image formats
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;

	//modes how images should be presented to screen
	std::vector<VkPresentModeKHR> PresentationModes;
};

struct SwapchainImage
{
	VkImage Image;
	VkImageView ImageView;
};

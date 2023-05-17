#pragma once

#include <cstdint>
#include <fstream>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

const uint32_t MAX_FRAME_DRAWS = 2;

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

static std::vector<char> ReadShaderFile(const std::string &filename)
{
	// open stream from given file
	//	"binary" -> read as binary
	//	"ate" -> start reading from end of file
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if(!file.is_open())
	{
		throw std::runtime_error("failed to open file at " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);

	// go to position 0
	file.seekg(0);
	file.read(fileBuffer.data(), fileSize);

	file.close();

	return fileBuffer;
}

static std::string GetPathLeaf(const fs::path& path)
{
	if(path.empty())
	{
		return "";
	}

	auto it = path.end();
	it--;

	return *it;
}

static fs::path GetShaderPath()
{
	// get shader path
	fs::path shaderPath = fs::current_path();
	std::string leaf = GetPathLeaf(shaderPath);

	while(!shaderPath.empty()
		&& strcmp(shaderPath.c_str(), "/") != 0
		&& strcmp(
			leaf.c_str(),
			"VulkanCourseApp") != 0)
	{

		shaderPath = shaderPath.parent_path();
		leaf = GetPathLeaf(shaderPath);
	}

	shaderPath /= "shaders";

	return shaderPath;
}

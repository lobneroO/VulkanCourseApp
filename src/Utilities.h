#pragma once

#include <cstdint>
#include <fstream>
#include <cstring>
#include <filesystem>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace fs = std::filesystem;

const uint32_t MAX_FRAME_DRAWS = 2;

const std::vector<const char*> DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// vertex data representation
struct Vertex
{
	// vertex position
	glm::vec3 Position;
	glm::vec3 Colour;
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

static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags propertyFlags)
{
	// get properties of _physical_ device memory
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for(uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        // sequentially go through every bit and check if it is set
        // if it is, also check the property flags we require are set, but no others are set
        if((allowedTypes & (1 << i))
            && (memProps.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
        {
            // this memory type is valid, therefore return the index
            return i;
        }
    }

    return 0;
}

static void CreateBufferAndAllocateMemory(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize bufferSize,
	VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	// the VkBuffer is just a description of the buffer contents, not the actual memory itself!
    // therefore, no memory is created/allocated here
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = bufferSize;
    createInfo.usage = bufferUsageFlags;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(logicalDevice, &createInfo, nullptr, buffer);

    if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    // get the buffer memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memRequirements);

    // allocate memory to buffer
    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memRequirements.size;
    // index of memory type on physical device that has required bit flags VK_MEMORY_PROPERTY_
    // HOST_VISIBLE_BIT:    CPU can interact with memory
    // HOST_COHERENT_BIT:   allows placement of data straight into buffer after mapping
    //                          (otherwise would need flushes and memory invalidating)
	// DEVICE_VISIBLE_BIT:	only GPU can interact with memory, has to be copied over from other buffer from CPU
    memAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits,
        bufferProperties);

    // allocate memory _on_ VkDeviceMemory
    result = vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, bufferMemory);
    if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    // bind memory _to_ given vertex buffer
    vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);
}

static void CopyBuffer(VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	// command buffer to hold transfer commands
	VkCommandBuffer transferCommandBuffer;

	// command buffer details
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = transferCommandPool;
	allocInfo.commandBufferCount = 1;

	// allocate command buffer from pool
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &transferCommandBuffer);

	// information to begin the command buffer record
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// we're only using the command buffer once, so setup for one time submit
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// begin recording transfer commands
	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);
	{
		// region of data to copy from and to
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = 0;
		bufferCopyRegion.dstOffset = 0;
		bufferCopyRegion.size = bufferSize;

		// command to copy src buffer to dst buffer
		vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);
	}
	vkEndCommandBuffer(transferCommandBuffer);

	// submit command buffer to queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;

	// submit transfer command to transfer queue and wait until it finishes
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	// free temporary command buffer back to pool
	vkFreeCommandBuffers(logicalDevice, transferCommandPool, 1, &transferCommandBuffer);
}

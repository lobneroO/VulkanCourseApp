#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::Mesh(const VkPhysicalDevice& newPhysicalDevice, const VkDevice& newDevice, std::vector<Vertex>* vertices)
{
    VertexCount = vertices->size();
    PhysicalDevice = newPhysicalDevice;
    LogicalDevice = newDevice;
    CreateVertexBuffer(vertices);
}

Mesh::~Mesh()
{

}

uint32_t Mesh::GetVertexCount() const
{
    return VertexCount;
}

VkBuffer Mesh::GetVertexBuffer() const
{
    return VertexBuffer;
}

void Mesh::DestroyVertexBuffer()
{
    vkDestroyBuffer(LogicalDevice, VertexBuffer, nullptr);
    vkFreeMemory(LogicalDevice, VertexBufferMemory, nullptr);
}

void Mesh::CreateVertexBuffer(std::vector<Vertex>* vertices)
{
    // Create Vertex Buffer

    // the VkBuffer is just a description of the buffer contents, not the actual memory itself!
    // therefore, no memory is created/allocated here
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = sizeof(Vertex) * vertices->size();
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(LogicalDevice, &createInfo, nullptr, &VertexBuffer);

    if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    // get the buffer memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(LogicalDevice, VertexBuffer, &memRequirements);

    // allocate memory to buffer
    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memRequirements.size;
    // index of memory type on physical device that has required bit flags
    // HOST_VISIBLE_BIT:    CPU can interact with memory
    // HOST_COHERENT_BIT:   allows placement of data straight into buffer after mapping
    //                          (otherwise would need flushes and memory invalidating)
    memAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // allocate memory _on_ VkDeviceMemory
    result = vkAllocateMemory(LogicalDevice, &memAllocInfo, nullptr, &VertexBufferMemory);
    if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    // bind memory _to_ given vertex buffer
    vkBindBufferMemory(LogicalDevice, VertexBuffer, VertexBufferMemory, 0);

    // Map Memory To Vertex Buffer

    // 1. create pointer to a point in normal memory
    void* data;

    // 2. map the vertex buffer memory to data pointer
    vkMapMemory(LogicalDevice, VertexBufferMemory, 0, createInfo.size, 0, &data);

    // 3. copy memory from vertices data to data pointer
    memcpy(data, vertices->data(), (size_t)(createInfo.size));

    // 4. unmap the vertex buffer memory
    //  we cannot be sure the memory is visible on the GPU yet,
    //  but it is guaranteed to be visible at the next vkQueueSubmit call
    vkUnmapMemory(LogicalDevice, VertexBufferMemory);
}

uint32_t Mesh::FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags propertyFlags)
{
    // get properties of _physical_ device memory
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProps);

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

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
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

    // create buffer and allocate memory to it
    CreateBufferAndAllocateMemory(PhysicalDevice, LogicalDevice, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &VertexBuffer, &VertexBufferMemory);

    // Map Memory To Vertex Buffer

    // 1. create pointer to a point in normal memory
    void* data;

    // 2. map the vertex buffer memory to data pointer
    vkMapMemory(LogicalDevice, VertexBufferMemory, 0, bufferSize, 0, &data);

    // 3. copy memory from vertices data to data pointer
    memcpy(data, vertices->data(), (size_t)(bufferSize));

    // 4. unmap the vertex buffer memory
    //  we cannot be sure the memory is visible on the GPU yet,
    //  but it is guaranteed to be visible at the next vkQueueSubmit call
    vkUnmapMemory(LogicalDevice, VertexBufferMemory);
}

#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::Mesh(const VkPhysicalDevice& newPhysicalDevice, const VkDevice& newDevice, VkQueue transferQueue,
         VkCommandPool transferCommandPool,  std::vector<Vertex>* vertices, std::vector<uint32_t>* indices)
{
    VertexCount = vertices->size();
    IndexCount = indices->size();
    PhysicalDevice = newPhysicalDevice;
    LogicalDevice = newDevice;
    CreateVertexBuffer(transferQueue, transferCommandPool, vertices);
    CreateIndexBuffer(transferQueue, transferCommandPool, indices);
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

uint32_t Mesh::GetIndexCount() const
{
    return IndexCount;
}

VkBuffer Mesh::GetIndexBuffer() const
{
    return IndexBuffer;
}

void Mesh::DestroyBuffers()
{
    vkDestroyBuffer(LogicalDevice, VertexBuffer, nullptr);
    vkFreeMemory(LogicalDevice, VertexBufferMemory, nullptr);

    vkDestroyBuffer(LogicalDevice, IndexBuffer, nullptr);
    vkFreeMemory(LogicalDevice, IndexBufferMemory, nullptr);
}

void Mesh::CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
    // Create Vertex Buffer
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

    // Host Coherent buffers are easy to use, since they are visible and usable by CPU and GPU
    // they are however not the most efficient. Device Local is better for the GPU, but cannot
    // be accessed by the CPU. Therefore, create a CPU visible buffer and transfer it to a GPU visible buffer

    // temporary buffer to "stage" vertex data before transferring to GPU:
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // create staging buffer and allocate memory to it
    CreateBufferAndAllocateMemory(PhysicalDevice, LogicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer, &stagingBufferMemory);

    // Map Memory To Staging Buffer

    // 1. create pointer to a point in normal memory
    void* data;

    // 2. map the vertex buffer memory to data pointer
    vkMapMemory(LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);

    // 3. copy memory from vertices data to data pointer
    memcpy(data, vertices->data(), (size_t)(bufferSize));

    // 4. unmap the vertex buffer memory
    //  we cannot be sure the memory is visible on the GPU yet,
    //  but it is guaranteed to be visible at the next vkQueueSubmit call
    vkUnmapMemory(LogicalDevice, stagingBufferMemory);

    // Create buffer with TRANSFER_DST bit (i.e. recipient of transfer (staging) buffer)
    // this is the actual vertex buffer
    // buffer memory is DEVICE_LOCAL, i.e. the GPU is accessing this only, not the CPU
    CreateBufferAndAllocateMemory(PhysicalDevice, LogicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &VertexBuffer, &VertexBufferMemory);

    // the buffer is transferred via the transfer "queue". as per vulkan definition, the graphics queue family has a transfer family
    // meaning we don't have to find one extra queue family
    CopyBuffer(LogicalDevice, transferQueue, transferCommandPool, stagingBuffer, VertexBuffer, bufferSize);

    // destroy staging buffer and free memory
    vkDestroyBuffer(LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(LogicalDevice, stagingBufferMemory, nullptr);
}

 void Mesh::CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
 {
    // Create Index Buffer
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

    // temporary buffer to "stage" vertex data before transferring to GPU:
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // create staging buffer and allocate memory to it
    CreateBufferAndAllocateMemory(PhysicalDevice, LogicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer, &stagingBufferMemory);

    void* data;
    vkMapMemory(LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices->data(), (size_t)(bufferSize));
    vkUnmapMemory(LogicalDevice, stagingBufferMemory);

    CreateBufferAndAllocateMemory(PhysicalDevice, LogicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &IndexBuffer, &IndexBufferMemory);

    CopyBuffer(LogicalDevice, transferQueue, transferCommandPool, stagingBuffer, IndexBuffer, bufferSize);

    // destroy staging buffer and free memory
    vkDestroyBuffer(LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(LogicalDevice, stagingBufferMemory, nullptr);
 }

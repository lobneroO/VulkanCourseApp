#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

class Mesh
{
public:
    Mesh();
    Mesh(const VkPhysicalDevice& newPhysicalDevice, const VkDevice& newDevice, VkQueue transferQueue,
         VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices);

    ~Mesh();

    uint32_t GetVertexCount() const;

    VkBuffer GetVertexBuffer() const;

    uint32_t GetIndexCount() const;

    VkBuffer GetIndexBuffer() const;

    void DestroyBuffers();

private:
    void CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
    void CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);

private:
    uint32_t VertexCount = 0;
    VkBuffer VertexBuffer;                  // the layout of the buffer, i.e. header, only information
    VkDeviceMemory VertexBufferMemory;      // actual memory

    uint32_t IndexCount = 0;
    VkBuffer IndexBuffer;
    VkDeviceMemory IndexBufferMemory;

    VkPhysicalDevice PhysicalDevice;
    VkDevice LogicalDevice;
};

#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

class Mesh
{
public:
    Mesh();
    Mesh(const VkPhysicalDevice& newPhysicalDevice, const VkDevice& newDevice, std::vector<Vertex>* vertices);

    ~Mesh();

    uint32_t GetVertexCount() const;

    VkBuffer GetVertexBuffer() const;

    void DestroyVertexBuffer();

private:
    void CreateVertexBuffer(std::vector<Vertex>* vertices);

private:
    uint32_t VertexCount = 0;
    VkBuffer VertexBuffer;                  // the layout of the buffer, i.e. header, only information
    VkDeviceMemory VertexBufferMemory;      // actual memory

    VkPhysicalDevice PhysicalDevice;
    VkDevice LogicalDevice;
};

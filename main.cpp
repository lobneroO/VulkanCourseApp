#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.h"

GLFWwindow* Window = nullptr;
VulkanRenderer Renderer;

int32_t InitWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
{
	//initialize glfw
	if (!glfwInit())
	{
		return EXIT_FAILURE;
	}

	//set glfw to not work with OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	Window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);

	return EXIT_SUCCESS;
}

int main()
{
	//create window
	if (InitWindow() == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	//create vulkan Renderer instance
	if (Renderer.Init(Window) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	//loop until closed
	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
	}

	Renderer.CleanUp();

	//destroy glfw window and stop glfw
	glfwDestroyWindow(Window);
	glfwTerminate();

	return 0;
}
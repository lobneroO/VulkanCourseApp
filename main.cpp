#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

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

	float angle_deg = 0.0f;
	float deltaTime_s = 0.0f;
	float lastTime = 0.0f;

	//loop until closed
	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime_s = now - lastTime;
		lastTime = now;

		angle_deg += 10.f * deltaTime_s;
		if(angle_deg > 360)
		{
			angle_deg -= 360;
		}

		Renderer.UpdateModel(glm::rotate(glm::mat4(1.f), glm::radians(angle_deg), glm::vec3(0.f, 0.f, 1.f)));

		Renderer.Draw();
	}

	Renderer.CleanUp();

	//destroy glfw window and stop glfw
	glfwDestroyWindow(Window);
	glfwTerminate();

	return 0;
}

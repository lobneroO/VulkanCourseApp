# cmake file for the Vulkan Course App developed with udemy

# setup the project
cmake_minimum_required(VERSION 3.10)
project(VulkanCourseApp VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src)

add_executable(VulkanCourseApp main.cpp)

target_link_libraries(VulkanCourseApp PUBLIC src)

target_include_directories(VulkanCourseApp PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
)

target_include_directories(VulkanCourseApp PUBLIC
    "${PROJECT_BINARY_DIR}"
)


# setup GLFW
# disable building docs, tests and examples when building GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(Imports/glfw-3.3.8/)

target_link_libraries(VulkanCourseApp PUBLIC glfw)

# setup Vulkan
find_package(Vulkan REQUIRED)

target_link_libraries(VulkanCourseApp PUBLIC ${Vulkan_LIBRARIES})
target_include_directories(VulkanCourseApp PUBLIC ${Vulkan_INCLUDE_DIR})


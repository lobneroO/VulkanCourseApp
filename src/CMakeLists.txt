
add_library(src VulkanRenderer.cpp)

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(src PRIVATE VulkanRenderer.cpp)

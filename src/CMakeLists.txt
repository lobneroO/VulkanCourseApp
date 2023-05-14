
add_library(VCASource VulkanRenderer.cpp)

target_include_directories(VCASource PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(VCASource PRIVATE VulkanRenderer.cpp)

cmake_minimum_required(VERSION 3.1)

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

if (WIN32)
	set(GLSL_VALIDATOR "${CMAKE_SOURCE_DIR}/include/VulkanSDK/win32/Bin32/glslangValidator.exe")
else()
	set(GLSL_VALIDATOR "${CMAKE_SOURCE_DIR}/include/VulkanSDK/x86_64/bin/glslangValidator")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# The version number.
set(${PROJECT_NAME}._VERSION_MAJOR 1)
set(${PROJECT_NAME}._VERSION_MINOR 0)
set(${PROJECT_NAME}._VERSION_BUILD 0)

file(GLOB_RECURSE SOURCE_FILES "source/*.cpp")
file(GLOB_RECURSE HEADER_FILES "source/*.h")
file(GLOB_RECURSE SHADER_FILES 
	"source/assets/shaders/*.frag"
	"source/assets/shaders/*.vert"
)

source_group("source" FILES ${SOURCE_FILES})
source_group("header" FILES ${HEADER_FILES})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

set_target_properties(${ProjectId} PROPERTIES LINKER_LANGUAGE CXX)

if (WIN32)
	target_link_libraries(${PROJECT_NAME}
		${CMAKE_SOURCE_DIR}/include/glfw-3.2.1/win32/glfw3.lib
		${CMAKE_SOURCE_DIR}/include/VulkanSDK/win32/Lib32/vulkan-1.lib
	)
else()
	target_link_libraries(${PROJECT_NAME}
		${CMAKE_SOURCE_DIR}/include/glfw-3.2.1/x86_64/libglfw.so
		${CMAKE_SOURCE_DIR}/include/VulkanSDK/x86_64/lib/libvulkan.so
	)
endif()

include_directories(
	${CMAKE_SOURCE_DIR}/include/VulkanSDK/include
	${CMAKE_SOURCE_DIR}/include/glfw-3.2.1/include
	${CMAKE_SOURCE_DIR}/include/glm-0.9.8.5
	${CMAKE_SOURCE_DIR}/include/gli
	${CMAKE_SOURCE_DIR}/include/stb
	${CMAKE_SOURCE_DIR}/include/tinyobjloader
)

foreach(GLSL ${SHADER_FILES})
	get_filename_component(FILE_NAME ${GLSL} NAME)
	set(SPIRV "${CMAKE_BINARY_DIR}/shaders/${FILE_NAME}.spv")
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
		COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
		DEPENDS ${GLSL}
	)
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
	Shaders
	DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(${PROJECT_NAME} Shaders)

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
# 	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/source/assets/shaders/spirv"
# 	COMMAND ${CMAKE_COMMAND} -E copy_directory
# 		"${CMAKE_BINARY_DIR}/shaders"
# 		"${CMAKE_SOURCE_DIR}/source/assets/shaders/spirv"
# )
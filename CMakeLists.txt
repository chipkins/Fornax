cmake_minimum_required(VERSION 3.1)

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

set(CMAKE_CXX_STANDARD 11)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# The version number.
set(${PROJECT_NAME}._VERSION_MAJOR 1)
set(${PROJECT_NAME}._VERSION_MINOR 0)
set(${PROJECT_NAME}._VERSION_BUILD 0)

file(GLOB_RECURSE SOURCE_FILES "source/*.cpp")
file(GLOB_RECURSE HEADER_FILES "source/*.h")

source_group("source" FILES ${SOURCE_FILES})
source_group("header" FILES ${HEADER_FILES})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

set_target_properties(${ProjectId} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME}
	${CMAKE_SOURCE_DIR}/include/glfw-3.2.1/src/libglfw.so
	${CMAKE_SOURCE_DIR}/include/VulkanSDK/x86_64/lib/libvulkan.so
)

include_directories(
	${CMAKE_SOURCE_DIR}/include/glfw-3.2.1/include
	${CMAKE_SOURCE_DIR}/include/glm-0.9.8.5
	${CMAKE_SOURCE_DIR}/include/VulkanSDK/x86_64/include
)

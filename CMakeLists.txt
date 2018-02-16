cmake_minimum_required(VERSION 3.7.1)

set(DEFAULT_BUILD_TYPE "Debug")
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

project(VulkanEngine)

file(GLOB INCLUDES ${INCLUDE_DIR}/*.hpp)
file(GLOB SOURCES ${SRC_DIR}/*.cpp)

set(CMAKE_CXX_STANDARD 14)

if(NOT LIB_VK_LIBRARY)
	message(WARNING "Vulkan library not found please specify it with -DLIB_VK_LIBRARY='Path to your vulkan lib' or use the cmake-gui to se set it")
	set(LIB_VK_LIBRARY "" CACHE FILEPATH "Path to your Vulkan library file")
	set(ERROR "YES")
endif()
if(NOT LIB_VK_INCLUDE_DIR)
	message(WARNING "Vulkan include directory not found please specify it with -DLIB_VK_INCLUDE_DIR='Path to your vulkan include dirrectory' or use the cmake-gui to se set it")
	set(LIB_VK_INCLUDE_DIR "" CACHE PATH "Path to your Vulkan include directory")
	set(ERROR "YES")
endif()

set(CMAKE_SUPPRESS_REGENERATION true)
set(VS_STARTUP_PROJECT "VulkanEngine")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if(ERROR)
	message(FATAL_ERROR "Error occured check output")
endif()

include_directories(${INCLUDE_DIR} ${LIB_VK_INCLUDE_DIR})

add_executable(VulkanEngine ${SOURCES} ${INCLUDES})

target_link_libraries(VulkanEngine ${LIB_VK_LIBRARY})

set_target_properties(VulkanEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")
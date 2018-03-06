cmake_minimum_required(VERSION 3.5.1)

################################################################
################    SET THE DEFAULT BUILD TO DEBUG
################################################################

set(DEFAULT_BUILD_TYPE "Debug")
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()



################################################################
################    SET SOME DIRECTORY CONSTANTS
################################################################

set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")



################################################################
################    CREATE PROJECT AND GET ALL SOURCE FILES
################################################################

project(VulkanEngine)
set(CMAKE_CXX_STANDARD 14)

file(GLOB INCLUDES ${INCLUDE_DIR}/*.hpp)
file(GLOB SOURCES ${SRC_DIR}/*.cpp)



################################################################
################    SET VULKAN SDK PATH
################################################################

if(NOT VK_SDK_PATH)
	message(WARNING "Vulkan SDK path not found")
	set(VK_SDK_PATH "" CACHE PATH "Path to your Vulkan SDK folder")
	set(ERROR "YES")
endif()



################################################################
################    SET VULKAN INCLUDES AND LIBRARY
################################################################

set(LIB_VK_INCLUDE_DIR "${VK_SDK_PATH}/include")
if(WIN32)
	set(LIB_VK_LIBRARY "${VK_SDK_PATH}/Lib/vulkan-1.lib")
endif()
if(UNIX)
	set(LIB_VK_LIBRARY "${VK_SDK_PATH}/lib/libvulkan.so")
endif()



################################################################
################    SET SHADERC INCLUDE AND LIBRARY
################################################################

if(WIN32)
	set(LIB_SHADERC_DEBUG "${VK_SDK_PATH}/shaderc/build/libshaderc/Debug/shaderc_combined.lib")
	set(LIB_SHADERC_RELEASE "${VK_SDK_PATH}/shaderc/build/libshaderc/Release/shaderc_combined.lib")
endif()
if(UNIX)
	set(LIB_SHADERC_DEBUG "${VK_SDK_PATH}/shaderc/Debug/libshaderc_shared.so")
	set(LIB_SHADERC_RELEASE "${VK_SDK_PATH}/shaderc/Release/libshaderc_shared.so")
endif()

################################################################
################    SET GLM INCLUDES
################################################################

if(NOT LIB_GLM_INCLUDE_DIR)
	message(WARNING "GLM library not found please specify it with -DLIB_GLM_INCLUDE_DIR='Path to your glm include dir' or use the cmake-gui to se set it")
	set(LIB_GLM_INCLUDE_DIR "" CACHE PATH "Path to your GLM include folder, should be 'glm'")
endif()



################################################################
################    SET SOME CMAKE VARIABLES
################################################################

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)



################################################################
################    IF SOME LIBRARY WAS NOT SPECIFIED
################################################################

if(ERROR)
	message(FATAL_ERROR "Error(s) occured check output")
endif()



################################################################
################    SPECIFY INCLUDE DIRECTORIES
################################################################

include_directories(${INCLUDE_DIR} ${LIB_VK_INCLUDE_DIR} ${LIB_GLM_INCLUDE_DIR})



################################################################
################    SPECIFY EXECUTABLE AND LINK LIBRARIES
################################################################

add_executable(VulkanEngine ${SOURCES} ${INCLUDES})
if (UNIX)
	target_link_libraries(VulkanEngine ${LIB_VK_LIBRARY} ${LIB_XCB} ${LIB_XAU} ${LIB_XDMCP})
endif(UNIX)

if (WIN32)
	target_link_libraries(VulkanEngine ${LIB_VK_LIBRARY})
endif (WIN32)

target_link_libraries (VulkanEngine
    debug ${LIB_SHADERC_DEBUG}
    optimized ${LIB_SHADERC_RELEASE})


################################################################
################    SET MSVC STARTUP PROJECT AND WORKING DIR
################################################################

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VulkanEngine)
set_target_properties(VulkanEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")
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
################    SET VULKAN INCLUDES AND LIBRARY
################################################################

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


################################################################
################    SET MSVC STARTUP PROJECT AND WORKING DIR
################################################################

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VulkanEngine)
set_target_properties(VulkanEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")
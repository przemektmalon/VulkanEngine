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

set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
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
################    SET BULLET INCLUDES
################################################################

if(NOT LIB_BULLET_INCLUDE_DIR)
	message(WARNING "Bullet library not found please specify it with -DLIB_BULLET_INCLUDE_DIR='Path to your bullet include dir' or use the cmake-gui to set it")
	set(LIB_BULLET_INCLUDE_DIR "" CACHE PATH "Path to your bullet include folder")
endif()
if(NOT LIB_BULLET_COLLISION_LIBRARY_DEBUG)
	message(WARNING "Debug bullet collision library not found please specify it with -DLIB_BULLET_COLLISION_LIBRARY_DEBUG='Path to your bullet collision library' or use the cmake-gui to set it")
	set(LIB_BULLET_COLLISION_LIBRARY_DEBUG "" CACHE FILEPATH "Path to your debug bullet collision library")
endif()
if(NOT LIB_BULLET_COLLISION_LIBRARY_RELEASE)
	message(WARNING "Release bullet collision library not found please specify it with -DLIB_BULLET_COLLISION_LIBRARY_RELEASE='Path to your bullet collision library' or use the cmake-gui to set it")
	set(LIB_BULLET_COLLISION_LIBRARY_RELEASE "" CACHE FILEPATH "Path to your release bullet collision library")
endif()

if(NOT LIB_BULLET_DYNAMICS_LIBRARY_DEBUG)
	message(WARNING "Debug bullet dynamics library not found please specify it with -DLIB_BULLET_DYNAMICS_LIBRARY_DEBUG='Path to your bullet dynamics library' or use the cmake-gui to set it")
	set(LIB_BULLET_DYNAMICS_LIBRARY_DEBUG "" CACHE FILEPATH "Path to your debug bullet dynamics library")
endif()
if(NOT LIB_BULLET_DYNAMICS_LIBRARY_RELEASE)
	message(WARNING "Release bullet dynamics library not found please specify it with -DLIB_BULLET_DYNAMICS_LIBRARY_RELEASE='Path to your bullet dynamics library' or use the cmake-gui to set it")
	set(LIB_BULLET_DYNAMICS_LIBRARY_RELEASE "" CACHE FILEPATH "Path to your release bullet dynamics library")
endif()

if(NOT LIB_BULLET_MATH_LIBRARY_DEBUG)
	message(WARNING "Debug bullet math library not found please specify it with -DLIB_BULLET_MATH_LIBRARY_DEBUG='Path to your bullet math library' or use the cmake-gui to set it")
	set(LIB_BULLET_MATH_LIBRARY_DEBUG "" CACHE FILEPATH "Path to your debug bullet math library")
endif()
if(NOT LIB_BULLET_MATH_LIBRARY_RELEASE)
	message(WARNING "Release bullet math library not found please specify it with -DLIB_BULLET_MATH_LIBRARY_RELEASE='Path to your bullet math library' or use the cmake-gui to set it")
	set(LIB_BULLET_MATH_LIBRARY_RELEASE "" CACHE FILEPATH "Path to your release bullet math library")
endif()


################################################################
################    SET GLM INCLUDES
################################################################

if(NOT LIB_GLM_INCLUDE_DIR)
	message(WARNING "GLM library not found please specify it with -DLIB_GLM_INCLUDE_DIR='Path to your glm include dir' or use the cmake-gui to set it")
	set(LIB_GLM_INCLUDE_DIR "" CACHE PATH "Path to your GLM include folder, should be 'glm'")
endif()



################################################################
################    SET ASSIMP & ZLIB INCLUDE AND LIBRARY
################################################################

if(NOT LIB_ASSIMP_LIBRARY_DEBUG)
	message(WARNING "ASSIMP debug library not found")
	set(LIB_ASSIMP_LIBRARY_DEBUG "" CACHE FILEPATH "Path to your debug ASSIMP lib")
	set(ERROR "YES")
endif()
if(NOT LIB_ASSIMP_LIBRARY_RELEASE)
	message(WARNING "ASSIMP release library not found")
	set(LIB_ASSIMP_LIBRARY_RELEASE "" CACHE FILEPATH "Path to your release ASSIMP lib")
	set(ERROR "YES")
endif()
if(NOT LIB_ASSIMP_INCLUDE_DIR)
	message(WARNING "ASSIMP include directory not found")
	set(LIB_ASSIMP_INCLUDE_DIR "" CACHE PATH "Path to your ASSIMP include folder")
	set(ERROR "YES")
endif()
if(WIN32)
	if(NOT LIB_ZLIB_LIBRARY_DEBUG)
		message(WARNING "ZLIB debug library not found")
		set(LIB_ZLIB_LIBRARY_DEBUG "" CACHE FILEPATH "Path to your ZLIB debug library")
		set(ERROR "YES")
	endif()
	if(NOT LIB_ZLIB_LIBRARY_RELEASE)
		message(WARNING "ZLIB release library not found")
		set(LIB_ZLIB_LIBRARY_RELEASE "" CACHE FILEPATH "Path to your ZLIB release library")
		set(ERROR "YES")
	endif()
endif()



################################################################
################    SET FREETYPE INCLUDE AND LIBRARY
################################################################

if(NOT LIB_FREETYPE_INCLUDE_DIR)
	message(WARNING "Freetype include dir not found please specify it with -DLIB_FREETYPE_INCLUDE_DIR='Path to your freetype include dir' or use the cmake-gui to set it")
	set(LIB_FREETYPE_INCLUDE_DIR "" CACHE PATH "Path to your freetype include folder")
endif()
if(NOT LIB_FREETYPE_LIB_DEBUG)
	message(WARNING "Freetype library not found please specify it with -DLIB_FREETYPE_INCLUDE_LIB_[DEBUG/RELEASE]='Path to your freetype DEBUG and RELEASE lib files' or use the cmake-gui to set it")
	set(LIB_FREETYPE_LIB_DEBUG "" CACHE FILEPATH "Path to your freetype debug library")
	set(LIB_FREETYPE_LIB_RELEASE "" CACHE FILEPATH "Path to your freetype release library")
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

include_directories(${INCLUDE_DIR} ${LIBS_DIR} ${LIB_BULLET_INCLUDE_DIR} ${LIB_FREETYPE_INCLUDE_DIR} ${LIB_VK_INCLUDE_DIR} ${LIB_GLM_INCLUDE_DIR} ${STB_INCLUDE_DIR} ${LIB_ASSIMP_INCLUDE_DIR})



################################################################
################    SPECIFY EXECUTABLE AND LINK LIBRARIES
################################################################

add_executable(VulkanEngine ${SOURCES} ${INCLUDES})
if (UNIX)
	target_link_libraries(VulkanEngine ${LIB_XCB} ${LIB_XAU} ${LIB_XDMCP})
endif(UNIX)

if (WIN32)
	target_link_libraries (VulkanEngine
    	debug ${LIB_ZLIB_LIBRARY_DEBUG}
    	optimized ${LIB_ZLIB_LIBRARY_RELEASE})
endif (WIN32)

target_link_libraries(VulkanEngine ${LIB_VK_LIBRARY})
target_link_libraries (VulkanEngine
	debug ${LIB_ASSIMP_LIBRARY_DEBUG}
	optimized ${LIB_ASSIMP_LIBRARY_RELEASE})
target_link_libraries (VulkanEngine
    debug ${LIB_SHADERC_DEBUG}
    optimized ${LIB_SHADERC_RELEASE})
target_link_libraries (VulkanEngine
    debug ${LIB_FREETYPE_LIB_DEBUG}
    optimized ${LIB_FREETYPE_LIB_RELEASE})

target_link_libraries (VulkanEngine
    debug ${LIB_BULLET_COLLISION_LIBRARY_DEBUG}
    optimized ${LIB_BULLET_COLLISION_LIBRARY_RELEASE})
target_link_libraries (VulkanEngine
    debug ${LIB_BULLET_DYNAMICS_LIBRARY_DEBUG}
    optimized ${LIB_BULLET_DYNAMICS_LIBRARY_RELEASE})
target_link_libraries (VulkanEngine
    debug ${LIB_BULLET_MATH_LIBRARY_DEBUG}
    optimized ${LIB_BULLET_MATH_LIBRARY_RELEASE})

################################################################
################    SET MSVC STARTUP PROJECT AND WORKING DIR
################################################################

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VulkanEngine)
set_target_properties(VulkanEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")
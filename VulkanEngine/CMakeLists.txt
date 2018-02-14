cmake_minimum_required(VERSION 3.7)

set(DEFAULT_BUILD_TYPE "Debug")
 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(PROJECT_NAME "${VulkanEngine}")

project(VulkanEngine)

file(GLOB INCLUDES ${INCLUDE_DIR}/*.hpp)
file(GLOB SROUCES ${SRC_DIR}/*.cpp)

set(CMAKE_CXX_STANDARD 14)

include_directories(${INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS})

add_executable(VulkanEngine ${SOURCES} ${INCLUDES})

target_link_libraries(VulkanEngine ${Vulkan_LIBRARIES})
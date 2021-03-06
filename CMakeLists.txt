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

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# To speed up initial compilation, configure assimp components we want to build (no tests or tools, only obj and gltf importers)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory("./lib/freetype")
add_subdirectory("./lib/vdu")
add_subdirectory("./lib/assimp")

set(LIB_GLM_INCLUDE_DIR 		"./lib/glm/")
set(LIB_XML_INCLUDE_DIR 		"./lib/rapidxml")
set(LIB_STB_INCLUDE_DIR 		"./lib/stb")
set(LIB_CHAI_INCLUDE_DIR 		"./lib/chaiscript/include")
set(LIB_VDU_INCLUDE_DIR 		"./lib/vdu/include")
set(LIB_FREETYPE_INCLUDE_DIR 	"./lib/freetype/include")
set(LIB_ASSIMP_INCLUDE_DIR 		"./lib/assimp/src")



################################################################
################    CREATE PROJECT AND GET ALL SOURCE FILES
################################################################

project(VulkanEngine)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${INCLUDE_DIR})
add_subdirectory(${SRC_DIR})

################################################################
################    FIND VULKAN AND SET SHADERC INCLUDE DIR AND LIBRARY
################################################################

find_package(Vulkan REQUIRED)

if (MSVC)
	set(SHADERC_ENABLE_SHARED_CRT ON CACHE BOOL "" FORCE)
endif (MSVC)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL ON CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE) # disables glslang CTestDashboard targets
set(RE2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EFFCEE_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(EFFCEE_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory("$ENV{VULKAN_SDK}/shaderc" "$ENV{VULKAN_SDK}/shaderc/build")



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
################    SET ZLIB INCLUDE AND LIBRARY (WIN32)
################################################################

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

include_directories(${INCLUDE_DIR} ${LIB_XML_INCLUDE_DIR} ${LIB_CHAI_INCLUDE_DIR} ${LIB_VDU_INCLUDE_DIR} ${LIB_BULLET_INCLUDE_DIR} ${LIB_FREETYPE_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS} ${LIB_GLM_INCLUDE_DIR} ${LIB_STB_INCLUDE_DIR} ${LIB_ASSIMP_INCLUDE_DIR})



################################################################
################    MACRO FOR GENERATING MSVC PRECOMPILED HEADERS
################    MODIFIED SLIGHTLY FROM SOURCE: https://stackoverflow.com/a/1380048
################################################################

MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "$(IntDir)$(TargetName).pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")  
    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)



################################################################
################    SPECIFY EXECUTABLE AND LINK LIBRARIES
################################################################

ADD_MSVC_PRECOMPILED_HEADER("PCH.hpp" "${SRC_DIR}/PCH.cpp" SOURCES)

add_executable(VulkanEngine ${SOURCES} ${INCLUDES})
if (UNIX)
	target_link_libraries(VulkanEngine ${LIB_XCB} ${LIB_XAU} ${LIB_XDMCP})
endif(UNIX)

if (WIN32)
	target_link_libraries (VulkanEngine
    	debug ${LIB_ZLIB_LIBRARY_DEBUG}
    	optimized ${LIB_ZLIB_LIBRARY_RELEASE})
endif (WIN32)

target_link_libraries(VulkanEngine assimp)
target_link_libraries(VulkanEngine ${Vulkan_LIBRARY})
target_link_libraries(VulkanEngine shaderc)
target_link_libraries(VulkanEngine freetype)
target_link_libraries(VulkanEngine vdu)

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
################    SET SOME MSVC FLAGS AND PROPERTIES
################################################################

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VulkanEngine)
set_target_properties(VulkanEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")

# For IDEs, place all dependency targets in a single folder so they dont clutter IDEs' solution trees
# I'm being a bit lazy here and ignoring the original libshaderc folder structure, just placing all the targets in a single folder

macro(TARGET_TO_FOLDER TARGET DEST_FOLDER)
    set_target_properties(${TARGET} PROPERTIES FOLDER ${DEST_FOLDER})
endmacro()

set(DEPENDENCIES 
    "glslang"
    "OGLCompiler"
    "OSDependent"
    "SPIRV"
    "HLSL"
		"spirv-tools-build-version"
		"spirv-tools-header-DebugInfo"
		"spirv-tools-shared-pkg-config"
    "spirv-tools-debuginfo"
    "spirv-tools-spv-amd-gcn-shader"
    "spirv-tools-spv-amd-shader-ballot"
    "spirv-tools-spv-amd-shader-explicit-vertex-parameter"
    "spirv-tools-spv-amd-shader-trinary-minmax"
    "SPIRV-Tools"
    "SPIRV-Tools-link"
    "SPIRV-Tools-opt"
    "SPIRV-Tools-reduce"
    "SPIRV-Tools-shared"
    "spirv-tools-vimsyntax"
		"add-copyright"
		"assimp"
    "build-version"
    "check-copyright"
    "core_tables"
    "enum_string_mapping"
    "extinst_tables"
    "freetype"
    "glslc"
    "glslc_exe"
    "IrrXML"
    "shaderc"
    "shaderc_combined_genfile"
    "shaderc_shared"
    "shaderc_util"
    "shaderc-online-compile"
    "spirv-tools-pkg-config"
    "testdata"
    "uninstall"
    "UpdateAssimpLibsDebugSymbolsAndDLLs"
    "vdu"
    "zlib"
		"zlibstatic")
		
foreach(DEP ${DEPENDENCIES})
	TARGET_TO_FOLDER(${DEP} "dependencies")
endforeach(DEP)

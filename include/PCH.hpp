
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <direct.h>
#define GetCurrentDir _getcwd
#endif
#ifdef __linux__
#define VK_USE_PLATFORM_XCB_KHR
#include "xcb/xcb.h"
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#include "vulkan/vulkan.h"
#include "Types.hpp"

#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "btBulletDynamicsCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"

#define CHAISCRIPT_NO_THREADS
#include "C:/Users/Przemek/Desktop/ChaiScript-6.0.0/include/chaiscript/chaiscript.hpp"

#define ENABLE_VULKAN_VALIDATION

#define VK_VALIDATE(f) { \
	f; \
	if (Engine::validationWarning) { \
		DBG_WARNING(Engine::validationMessage); \
		Engine::validationWarning = false; \
		Engine::validationMessage.clear(); \
	} \
}

#define VK_VALIDATE_W_RESULT(f) \
	auto result = f; \
	if (Engine::validationWarning) { \
			DBG_WARNING(Engine::validationMessage); \
			Engine::validationWarning = false; \
			Engine::validationMessage.clear(); \
	}

#define VK_CHECK_RESULT(f) { \
	VK_VALIDATE_W_RESULT(f); \
	if (result != VK_SUCCESS) { \
		switch(result){ \
			case(VK_ERROR_OUT_OF_HOST_MEMORY): \
				DBG_SEVERE("VK_ERROR_OUT_OF_HOST_MEMORY"); break; \
			case(VK_ERROR_OUT_OF_DEVICE_MEMORY): \
				DBG_SEVERE("VK_ERROR_OUT_OF_DEVICE_MEMORY"); break; \
			case(VK_ERROR_INITIALIZATION_FAILED): \
				DBG_SEVERE("VK_ERROR_INITIALIZATION_FAILED"); break; \
			case(VK_ERROR_DEVICE_LOST): \
				DBG_SEVERE("VK_ERROR_DEVICE_LOST"); break; \
			case(VK_ERROR_MEMORY_MAP_FAILED): \
				DBG_SEVERE("VK_ERROR_MEMORY_MAP_FAILED"); break; \
			case(VK_ERROR_LAYER_NOT_PRESENT): \
				DBG_SEVERE("VK_ERROR_LAYER_NOT_PRESENT"); break; \
			case(VK_ERROR_EXTENSION_NOT_PRESENT): \
				DBG_SEVERE("VK_ERROR_EXTENSION_NOT_PRESENT"); break; \
			case(VK_ERROR_FEATURE_NOT_PRESENT): \
				DBG_SEVERE("VK_ERROR_FEATURE_NOT_PRESENT"); break; \
			case(VK_ERROR_INCOMPATIBLE_DRIVER): \
				DBG_SEVERE("VK_ERROR_INCOMPATIBLE_DRIVER"); break; \
			case(VK_ERROR_TOO_MANY_OBJECTS): \
				DBG_SEVERE("VK_ERROR_TOO_MANY_OBJECTS"); break; \
			case(VK_ERROR_FORMAT_NOT_SUPPORTED): \
				DBG_SEVERE("VK_ERROR_FORMAT_NOT_SUPPORTED"); break; \
			case(VK_ERROR_FRAGMENTED_POOL): \
				DBG_SEVERE("VK_ERROR_FRAGMENTED_POOL"); break; \
			default: \
				DBG_SEVERE("VK_ERROR"); break; \
		} \
	} \
}

// Debugging output macros
#ifdef _WIN32

	/*
	bit 0 - foreground blue
	bit 1 - foreground green
	bit 2 - foreground red
	bit 3 - foreground intensity

	bit 4 - background blue
	bit 5 - background green
	bit 6 - background red
	bit 7 - background intensity
	*/

	#ifdef _MSC_VER
		#define DBG_SEVERE(msg) { \
			std::stringstream ss; \
			ss << msg; \
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b11001111); \
			std::cout << __FILE__ << " Line: " << __LINE__ << std::endl; \
			std::cout << "! SEVERE  -"; \
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00001111); \
			std::cout << " " << ss.str() << std::endl; \
			MessageBoxA(NULL, LPCSTR(ss.str().c_str()), "Severe Error!", MB_OK); \
			DebugBreak(); } 
	#else
		#define DBG_SEVERE(msg) { \
			std::stringstream ss; \
			ss << msg; \
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b11001111); \
			std::cout << "! SEVERE  -"; \
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00001111); \
			std::cout << " " << ss.str() << std::endl; \
			MessageBoxA(NULL, LPCSTR(ss.str().c_str()), "Severe Error!", MB_OK); \
			__builtin_trap(); }
	#endif
	#define DBG_WARNING(msg) { \
		std::stringstream ss; \
		ss << msg; \
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b01101111); \
		std::cout << __FILE__ << " Line: " << __LINE__ << std::endl; \
		std::cout << "! Warning -"; \
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00001111); \
		std::cout << " " << ss.str() << std::endl; }

	#define DBG_INFO(msg) { \
		std::stringstream ss; \
		ss << msg; \
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00011111); \
		std::cout << " Info     -"; \
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0b00001111); \
		std::cout << " " << ss.str() << std::endl; }
#else
	#ifdef __linux__
		#define DBG_SEVERE(msg) { \
			std::stringstream ss; \
			ss << msg; \
			std::cout << "\033[1;31m! SEVERE  -\033[0m"; \
			std::cout << " " << ss.str() << std::endl; }
		#define DBG_WARNING(msg) { \
			std::stringstream ss; \
			ss << msg; \
			std::cout << "\033[1;33m! WARNING  -\033[0m"; \
			std::cout << " " << ss.str() << std::endl; }
	
		#define DBG_INFO(msg) { \
			std::stringstream ss; \
			ss << msg; \
			std::cout << "\033[1;32m! INFO  -\033[0m"; \
			std::cout << " " << ss.str() << std::endl; }
	#endif
#endif




/// Utilities includes

#include <cstdlib>
// system, getenv, malloc, calloc, realloc, free, atof, atoi, abs

#include <csignal>
// signal handlers

#include <functional>

#include <utility>
// pair, make_pair

#include <ctime>

#include <chrono>

#include <initializer_list>

/// Numeric limits includes

#include <limits>

/// Error handling includes

#include <cassert>

/// Strings includes

#include <string>

/// Containers includes

#include <vector>

#include <list>

#include <set>

#include <unordered_set>

#include <map>

#include <unordered_map>

#include <queue>

#include <array>

/// Algorithms includes

#include <algorithm>

/// Iterators includes

#include <iterator>

/// Numerics includes

#include <cmath>

#include <random>

/// IO includes

#include <ios>

#include <iostream>

#include <fstream>

#include <cstdio>

#include <sstream>

/// Regex includes

#include <regex>

///  Thread support

#include <thread>
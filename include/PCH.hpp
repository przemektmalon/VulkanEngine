
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

#define ENABLE_VULKAN_VALIDATION

#ifndef ENABLE_VULKAN_VALIDATION
#define VDU_NO_VALIDATION
#endif

#include "vdu/PCH.hpp"

#define VK_VALIDATE(f) VDU_VK_VALIDATE(f)
#define VK_CHECK_RESULT(f) VDU_VK_CHECK_RESULT(f)

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
#include "glm/gtx/vector_angle.hpp"

#include "btBulletDynamicsCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"

#define CHAISCRIPT_NO_THREADS
#include "chaiscript/chaiscript.hpp"
#include <typeinfo>

#undef DBG_SEVERE
#undef DBG_WARNING
#undef DBG_INFO

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

#include <atomic>

/// Numeric limits includes

#include <limits>

/// Error handling includes

#include <cassert>

/// Strings includes

#include <string>

/// Containers includes

#include <vector>

#include <list>

#include <forward_list>

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

#include <mutex>
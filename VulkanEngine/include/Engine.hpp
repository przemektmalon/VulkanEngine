#pragma once
#include "PCH.hpp"

class Window;

/*

	App launch.
	Construct win32 window.
	! Construct vulkan instance, devices, surfaces etc

*/

/*
	@brief Main engine class that collates the engine sub-classes and sub-systems
*/
class Engine
{
public:
	Engine() {}
	~Engine() {}

#ifdef _WIN32
	static HINSTANCE win32InstanceHandle;
#endif
	static Window* window;
	static VkInstance vkInstance;
	static bool engineRunning;

	static void start();
	static void createWindow();
	static void initVulkanInstance();
	static void quit();

#ifdef ENABLE_VULKAN_VALIDATION
	static VkDebugReportCallbackEXT debugCallbackInfo;
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
#endif
};
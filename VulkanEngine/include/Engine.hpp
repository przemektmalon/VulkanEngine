#pragma once
#include "PCH.hpp"
#include "VulkanWrappers.hpp"

class Window;
class Renderer;

/*
	@brief Main engine class that collates the engine sub-classes and sub-systems
*/
class Engine
{
public:
	Engine() {}
	~Engine() {}

	static void start();
	static void createVulkanInstance();
	static void queryVulkanPhysicalDeviceDetails();
	static void createWindow();
	static void quit();

	static VkPhysicalDevice getPhysicalDevice() { return vkPhysicalDevice; }
	static PhysicalDeviceDetails& getPhysicalDeviceDetails() { return physicalDevicesDetails[physicalDeviceIndex]; }

#ifdef ENABLE_VULKAN_VALIDATION
	static VkDebugReportCallbackEXT debugCallbackInfo;
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
#endif

#ifdef _WIN32
	static HINSTANCE win32InstanceHandle;
#endif
	static Window* window;
	static Renderer* renderer;

	static VkInstance vkInstance;
	static VkPhysicalDevice vkPhysicalDevice;
	static std::vector<PhysicalDeviceDetails> physicalDevicesDetails;
	static int physicalDeviceIndex;
	
	static bool engineRunning;
};
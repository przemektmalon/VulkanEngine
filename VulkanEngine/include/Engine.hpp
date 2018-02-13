#pragma once
#include "PCH.hpp"

class Window;

/*

	App launch.
	Construct win32 window.
	Construct vulkan instance, devices, surfaces etc
	! Create vulkan swapchain
*/

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

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
	static VkPhysicalDevice vkPhysicalDevice;
	static VkDevice vkDevice;
	static VkQueue vkGraphicsQueue;
	static VkQueue vkPresentQueue;
	static VkSwapchainKHR vkSwapChain;
	static std::vector<VkImage> vkSwapChainImages;
	static VkFormat swapChainImageFormat;
	static VkExtent2D swapChainExtent;
	static bool engineRunning;

	static void start();
	static void createWindow();
	static void initVulkanInstance();
	static void pickVulkanPhysicalDevice();
	static void initVulkanLogicalDevice();
	static void initVulkanSwapChain();
	static void quit();

#ifdef ENABLE_VULKAN_VALIDATION
	static VkDebugReportCallbackEXT debugCallbackInfo;
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
#endif
};
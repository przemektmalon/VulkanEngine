#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"

/*
	@brief	Initialise enigne and sub-systems
*/
void Engine::start()
{
#ifdef _WIN32
	win32InstanceHandle = GetModuleHandle(NULL);
#endif

	createWindow();
	initVulkanInstance();

	while (engineRunning)
	{
		if (!window->processMessages())
		{

		}
	}

	quit();
}

/*
	@brief	Create a window for drawing to
*/
void Engine::createWindow()
{
	// Window creator requires OS specific handles
	WindowCreateInfo wci;
#ifdef _WIN32
	wci.win32InstanceHandle = win32InstanceHandle;
#endif
	wci.height = 720;
	wci.width = 1280;
	wci.posX = 100;
	wci.posY = 100;
	wci.title = "Vulkan Engine";
	wci.borderless = false;

	window = new Window;
	window->create(&wci);
}

/*
	@brief	Creates vulkan instance, enables extensions
	@note	If ENABLE_VULKAN_VALIDATION is defined, adds lunarg validation layer 
*/
void Engine::initVulkanInstance()
{
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = 0;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pApplicationName = "VulkanEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "My Vulkan Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	std::vector<const char *> enabledExtensions;
	enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#ifdef ENABLE_VULKAN_VALIDATION
	enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo instInfo;
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instInfo.pNext = 0;
	instInfo.pApplicationInfo = &appInfo;
	instInfo.flags = 0;
	instInfo.enabledExtensionCount = enabledExtensions.size();
	instInfo.ppEnabledExtensionNames = &enabledExtensions[0];
#ifdef ENABLE_VULKAN_VALIDATION
	instInfo.enabledLayerCount = 1;
	auto layerName = "VK_LAYER_LUNARG_standard_validation";
	instInfo.ppEnabledLayerNames = &layerName;
#elif
	instInfo.enabledLayerCount = 0;
	instInfo.ppEnabledLayerNames = 0;
#endif

	if (vkCreateInstance(&instInfo, nullptr, &vkInstance) != VK_SUCCESS)
		DBG_SEVERE("Could not create Vulkan instance");

#ifdef ENABLE_VULKAN_VALIDATION
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallbackFunc;

	auto createDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugReportCallbackEXT");

	if (createDebugReportCallbackEXT(vkInstance, &createInfo, nullptr, &debugCallbackInfo) != VK_SUCCESS) {
		DBG_WARNING("Failed to create debug callback");
	}
#endif

	
}

void Engine::quit()
{
	vkDestroyInstance(vkInstance, nullptr);
}

#ifdef ENABLE_VULKAN_VALIDATION
VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallbackFunc(VkDebugReportFlagsEXT flags,
													VkDebugReportObjectTypeEXT objType,
													uint64_t obj,
													size_t location,
													int32_t code,
													const char* layerPrefix,
													const char* msg,
													void* userData)
{
	DBG_WARNING(msg);
	return VK_FALSE;
}
VkDebugReportCallbackEXT Engine::debugCallbackInfo;
#endif

#ifdef _WIN32
HINSTANCE Engine::win32InstanceHandle;
#endif
Window* Engine::window;
VkInstance Engine::vkInstance;
bool Engine::engineRunning = true;
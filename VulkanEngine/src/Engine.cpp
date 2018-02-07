#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"

/*
	@brief	Initialise enigne and sub-systems
*/
void Engine::start()
{
	DBG_INFO("Launching engine");
#ifdef _WIN32
	win32InstanceHandle = GetModuleHandle(NULL);
#endif

	initVulkanInstance();
	createWindow();
	pickVulkanPhysicalDevice();
	initVulkanLogicalDevice();

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
	DBG_INFO("Creating window");
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
	DBG_INFO("Creating vulkan instance");
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
	DBG_INFO("Creating validation layer debug callback");
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

/*
	@brief	Queries vulkan instance for physical device information, picks best one
*/
void Engine::pickVulkanPhysicalDevice()
{
	DBG_INFO("Picking Vulkan device")
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
		DBG_SEVERE("No GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

	for (const auto& device : devices) 
	{
		bool deviceSuitable = false;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;

		for (const auto& queueFamily : queueFamilies)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, window->vkSurface, &presentSupport);
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport)
			{
				deviceSuitable = true;
				break;
			}
			++i;
		}

		if (deviceSuitable)
		{
			vkPhysicalDevice = device;
			break;
		}
	}

	if (vkPhysicalDevice == VK_NULL_HANDLE)
		DBG_SEVERE("Faled to find a suitable GPU");
}

/*
	@brief	Creates a Vulkan logical device (from the *best* physical device) with specific features and queues
*/
void Engine::initVulkanLogicalDevice()
{
	DBG_INFO("Creating Vulkan logical device");
	VkDeviceQueueCreateInfo qci;
	qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qci.pNext = 0;
	qci.flags = 0;
	qci.queueFamilyIndex = 0;
	qci.queueCount = 1;
	float priority = 1.f;
	qci.pQueuePriorities = &priority;

	VkPhysicalDeviceFeatures pdf = {};

	VkDeviceCreateInfo dci;
	dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.pNext = 0;
	dci.flags = 0;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
#ifdef ENABLE_VULKAN_VALIDATION
	dci.enabledLayerCount = 1;
	auto layerName = "VK_LAYER_LUNARG_standard_validation";
	dci.ppEnabledLayerNames = &layerName;
#elif
	dci.enabledLayerCount = 0;
	dci.ppEnabledLayerNames = 0;
#endif
	dci.enabledExtensionCount = 0;
	dci.ppEnabledExtensionNames = 0;
	dci.pEnabledFeatures = &pdf;

	if (vkCreateDevice(vkPhysicalDevice, &dci, 0, &vkDevice) != VK_SUCCESS)
		DBG_SEVERE("Could not create Vulkan logical device");

	vkGetDeviceQueue(vkDevice, 0, 0, &vkGraphicsQueue);
	vkGetDeviceQueue(vkDevice, 0, 0, &vkPresentQueue); /// Todo: Support for AMD gpus
}

/*
	@brief	Cleanup memory on exit
*/
void Engine::quit()
{
	DBG_INFO("Exiting");
#ifdef ENABLE_VULKAN_VALIDATION
	PFN_vkDestroyDebugReportCallbackEXT(vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugReportCallbackEXT"))(vkInstance, debugCallbackInfo, 0);
#endif
	window->destroy();
	vkDestroyInstance(vkInstance, nullptr);
	vkDestroyDevice(vkDevice, nullptr);
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
VkPhysicalDevice Engine::vkPhysicalDevice = VK_NULL_HANDLE;
VkDevice Engine::vkDevice;
VkQueue Engine::vkGraphicsQueue;
bool Engine::engineRunning = true;
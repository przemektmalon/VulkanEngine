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
	initVulkanSwapChain();
	initVulkanImageViews();

	while (engineRunning)
	{
		while (window->processMessages()) { /* Invoke timer ? */ }
		
		// Rendering and engine logic
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
	dci.enabledExtensionCount = 1;
	auto deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME; /// Todo: check support function
	dci.ppEnabledExtensionNames = &deviceExtension;
	dci.pEnabledFeatures = &pdf;

	if (vkCreateDevice(vkPhysicalDevice, &dci, 0, &vkDevice) != VK_SUCCESS)
		DBG_SEVERE("Could not create Vulkan logical device");

	vkGetDeviceQueue(vkDevice, 0, 0, &vkGraphicsQueue);
	vkGetDeviceQueue(vkDevice, 0, 0, &vkPresentQueue); /// Todo: Support for AMD gpus which have non-universal queues
}

/*
	@brief	Create the optimal vulkan swapchain
*/
void Engine::initVulkanSwapChain()
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, window->vkSurface, &details.capabilities);

	u32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, window->vkSurface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, window->vkSurface, &formatCount, details.formats.data());
	}
	else
	{
		DBG_SEVERE("Device not suitable (swap chain format count is 0)");
	}

	u32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, window->vkSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, window->vkSurface, &presentModeCount, details.presentModes.data());
	}
	else
	{
		DBG_SEVERE("Device not suitable (swap chain present modes count is 0)");
	}

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D extent;

	if (details.formats.size() == 1 && details.formats[0].format == VK_FORMAT_UNDEFINED) {
		surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	else
	{
		bool foundSuitable = false;
		for (const auto& availableFormat : details.formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surfaceFormat = availableFormat;
				foundSuitable = true;
			}
		}
		if (!foundSuitable)
			surfaceFormat = details.formats[0];
	}

	for (const auto& availablePresentMode : details.presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
			break;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentMode = availablePresentMode;
		}
	}

	if (details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = details.capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { window->resX, window->resY };

		actualExtent.width = std::max(details.capabilities.minImageExtent.width, std::min(details.capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(details.capabilities.minImageExtent.height, std::min(details.capabilities.maxImageExtent.height, actualExtent.height));

		extent = actualExtent;
	}

	u32 imageCount = details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
		imageCount = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = window->vkSurface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0; // Optional
	createInfo.pQueueFamilyIndices = nullptr; // Optional

	/*QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}*/

	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &vkSwapChain) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan swap chain!");
	}

	vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, nullptr);
	vkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, vkSwapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

/*
	@brief	Create image views for swap chain images
*/
void Engine::initVulkanImageViews()
{
	vkSwapChainImageViews.resize(vkSwapChainImages.size());

	for (size_t i = 0; i < vkSwapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = vkSwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(vkDevice, &createInfo, nullptr, &vkSwapChainImageViews[i]) != VK_SUCCESS) {
			DBG_SEVERE("Failed to create vulkan image views");
		}
	}
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

	for (auto imageView : vkSwapChainImageViews) {
		vkDestroyImageView(vkDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);
	window->destroy();
	vkDestroyDevice(vkDevice, nullptr);
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
VkPhysicalDevice Engine::vkPhysicalDevice = VK_NULL_HANDLE;
VkDevice Engine::vkDevice;
VkQueue Engine::vkGraphicsQueue;
VkQueue Engine::vkPresentQueue;
VkSwapchainKHR Engine::vkSwapChain;
std::vector<VkImage> Engine::vkSwapChainImages;
std::vector<VkImageView> Engine::vkSwapChainImageViews;
VkFormat Engine::swapChainImageFormat;
VkExtent2D Engine::swapChainExtent;
bool Engine::engineRunning = true;
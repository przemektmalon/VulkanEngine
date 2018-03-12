#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Renderer.hpp"

#include "Image.hpp"

#include "cro_mipmap.h"

#include "Profiler.hpp"

/*
	@brief	Initialise enigne and sub-systems
*/
void Engine::start()
{
	DBG_INFO("Launching engine");
	engineStartTime = clock.time();
#ifdef _WIN32
	win32InstanceHandle = GetModuleHandle(NULL);
#endif
#ifdef __linux__
	connection = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(connection))
		DBG_SEVERE("Failed to connect to X server using XCB");
#endif

	createVulkanInstance();
	createWindow();
	queryVulkanPhysicalDeviceDetails();

	camera.initialiseProj(float(window->resX) / float(window->resY), glm::pi<float>() / 3.f, 0.1, 50.f);
	renderer = new Renderer();
	renderer->initialise();

	Time initTime = clock.time() - engineStartTime;
	DBG_INFO("Initialisation time: " << initTime.getSecondsf() << " seconds");

	Time frameTime;
	double fpsDisplay = 0.f;
	int frames = 0;
	while (engineRunning)
	{
		frameTime = clock.time();

		while (window->processMessages()) { /* Invoke timer ? */ }
		
#ifdef _WIN32
		GetKeyboardState(Keyboard::keyState);
#endif

		Event ev;
		while (window->eventQ.pollEvent(ev)) {
			switch (ev.type) {
			case(Event::MouseMove):
			{
				if (ev.eventUnion.mouseEvent.code & Mouse::M_RIGHT)
				{
					camera.rotate(0, 0.0035 * ev.eventUnion.mouseEvent.move.y, 0.0035 * ev.eventUnion.mouseEvent.move.x);
					/// TODO: set mouse position to be locked to middle of window
				}
				break;
			}
			case Event::KeyDown: {
				if (ev.eventUnion.keyEvent.key.code == Key::KC_ESCAPE)
					engineRunning = false;
				std::cout << "Key down: " << char(ev.eventUnion.keyEvent.key.code) << std::endl;

				if (ev.eventUnion.keyEvent.shift && ev.eventUnion.keyEvent.key.code == Key::KC_W)
					std::cout << "SHIFT+W" << std::endl;
				break;
			}
			case Event::KeyUp: {
				std::cout << "Key up: " << char(ev.eventUnion.keyEvent.key.code) << std::endl;
				break;
			}
			}
		}
		
		
		// Rendering and engine logic
		renderer->updateUniformBuffer();
		renderer->render();

		frameTime = clock.time() - frameTime;

		float camSpeed = 5.f;

		auto move = glm::fvec3(glm::fvec4(0, 0, camSpeed * frameTime.getSeconds(), 1) * camera.getMatYaw());
		camera.move(-move * float(Keyboard::isKeyPressed('W')));

		move = glm::cross(glm::fvec3(glm::fvec4(0, 0, camSpeed * frameTime.getSeconds(), 1) * camera.getMatYaw()), glm::fvec3(0, 1, 0));
		camera.move(move * float(Keyboard::isKeyPressed('A')));

		move = glm::fvec3(glm::fvec4(0, 0, camSpeed * frameTime.getSeconds(), 1) * camera.getMatYaw());
		camera.move(move * float(Keyboard::isKeyPressed('S')));

		move = glm::cross(glm::fvec3(glm::fvec4(0, 0, camSpeed * frameTime.getSeconds(), 1) * camera.getMatYaw()), glm::fvec3(0, 1, 0));
		camera.move(-move * float(Keyboard::isKeyPressed('D')));

		camera.move(glm::fvec3(0,  camSpeed * frameTime.getSeconds() * float(Keyboard::isKeyPressed('R')),0));
		camera.move(glm::fvec3(0, -camSpeed * frameTime.getSeconds() * float(Keyboard::isKeyPressed('F')), 0));

		camera.update(frameTime);

		// FPS display
		++frames;
		fpsDisplay += frameTime.getSeconds();
		if (fpsDisplay > 1.f)
		{
			//printf("%f\n", double(frames) / fpsDisplay);
			fpsDisplay = 0.f;
			frames = 0;
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
#ifdef __linux__
	wci.connection = connection;
#endif
	wci.width = 1600;
	wci.height = 900;
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
void Engine::createVulkanInstance()
{
	DBG_INFO("Creating vulkan instance");
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = 0;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pApplicationName = "VulkanEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "My Vulkan Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	std::vector<const char *> enabledExtensions;
	enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef __linux__
	enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#ifdef ENABLE_VULKAN_VALIDATION
	enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo instInfo = {};
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
#else
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
void Engine::queryVulkanPhysicalDeviceDetails()
{
	DBG_INFO("Picking Vulkan device");
		uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
		DBG_SEVERE("No GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		physicalDevicesDetails.push_back(PhysicalDeviceDetails(device));

		auto& qDevice = physicalDevicesDetails.back();

		qDevice.queryDetails();	
	}

	s32 highestSuitabilityScore = 0;
	int i = 0;
	for (auto& device : physicalDevicesDetails)
	{
		if (device.suitabilityScore > highestSuitabilityScore)
		{
			highestSuitabilityScore = device.suitabilityScore;
			vkPhysicalDevice = device.vkPhysicalDevice;
			physicalDeviceIndex = i;
		}
		++i;
	}

	if (vkPhysicalDevice == VK_NULL_HANDLE)
		DBG_SEVERE("Faled to find a suitable GPU");
}

/*
	@brief	Cleanup memory on exit
*/
void Engine::quit()
{
	DBG_INFO("Exiting");

	renderer->cleanup();
	window->destroy();
#ifdef ENABLE_VULKAN_VALIDATION
	PFN_vkDestroyDebugReportCallbackEXT(vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugReportCallbackEXT"))(vkInstance, debugCallbackInfo, 0);
#endif
	vkDestroyInstance(vkInstance, nullptr);
}

#ifdef ENABLE_VULKAN_VALIDATION
/*
	@brief	Vulkan validation. Debugging output.
*/
VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallbackFunc(VkDebugReportFlagsEXT flags,
													VkDebugReportObjectTypeEXT objType,
													uint64_t obj,
													size_t location,
													int32_t code,
													const char* layerPrefix,
													const char* msg,
													void* userData)
{
	DBG_WARNING(msg << "\n" << "        NOTE: The line numbers are incorrect for Vulkan warnings");
	return VK_FALSE;
}
VkDebugReportCallbackEXT Engine::debugCallbackInfo;
#endif

#ifdef _WIN32
HINSTANCE Engine::win32InstanceHandle;
#endif
#ifdef __linux__
xcb_connection_t * Engine::connection;
#endif
Clock Engine::clock;
Window* Engine::window;
Renderer* Engine::renderer;
VkInstance Engine::vkInstance;
VkPhysicalDevice Engine::vkPhysicalDevice = VK_NULL_HANDLE;
std::vector<PhysicalDeviceDetails> Engine::physicalDevicesDetails;
int Engine::physicalDeviceIndex;
bool Engine::engineRunning = true;
Time Engine::engineStartTime;
Camera Engine::camera;
#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include "Image.hpp"
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

	PROFILE_START("init");

	createVulkanInstance();
	createWindow();
	queryVulkanPhysicalDeviceDetails();

	maxDepth = 5000.f;
	
	camera.initialiseProj(float(window->resX) / float(window->resY), glm::pi<float>() / 2.f, 0.1, maxDepth);

	renderer = new Renderer();
	renderer->initialise();

	PROFILE_END("init");

	PROFILE_START("assets");

	Engine::assets.loadAssets("res/resources.txt");

	PROFILE_END("assets");

	PROFILE_START("descsets");

	renderer->updateGBufferDescriptorSets();
	renderer->updatePBRDescriptorSets();
	renderer->updateScreenDescriptorSets();

	PROFILE_END("descsets");

	PROFILE_START("world");

	// Adding models to the world
	{
		std::string materialList[6] = { "bamboo", "greasymetal", "marble", "dirt", "mahogany", "copper" };

		for (int i = 0; i < 25; ++i)
		{
			world.addModelInstance("hollowbox", "hollowbox" + std::to_string(i));
			world.modelMap["hollowbox" + std::to_string(i)]->transform = glm::translate(glm::fmat4(1), glm::fvec3(-((i % 5) * 2), std::floor(int(i / (int)5) * 2), 0));
			world.modelMap["hollowbox" + std::to_string(i)]->setMaterial(0, 0, assets.getMaterial(materialList[i % 6]));
			//if (i == 0)
				//world.modelMap["hollowbox" + std::to_string(i)]->transform = glm::scale(glm::fmat4(1), glm::fvec3(500, 500, 500));
		}

		world.addModelInstance("pbrsphere");
		world.modelMap["pbrsphere"]->transform = glm::translate(glm::fmat4(1), glm::fvec3(4, 0, 0));

		world.addModelInstance("ground");
		world.modelMap["ground"]->transform = glm::translate(glm::fmat4(1), glm::fvec3(0, -1, 0));
		world.modelMap["ground"]->setMaterial(0, 0, assets.getMaterial("dirt"));
	}

	PROFILE_END("world");

	PROFILE_START("cmds");

	renderer->populateDrawCmdBuffer();

	renderer->updateGBufferCommands();
	renderer->updatePBRCommands();
	renderer->updateScreenCommands();

	PROFILE_END("cmds");

	std::cout << std::endl;
	DBG_INFO("Initialisation time:     " << PROFILE_TIME("init") << " seconds");
	DBG_INFO("Asset load time:         " << PROFILE_TIME("assets") << " seconds");
	DBG_INFO("Descriptor sets time:    " << PROFILE_TIME("descsets") << " seconds");
	DBG_INFO("World loading time:      " << PROFILE_TIME("world") << " seconds");
	DBG_INFO("Command submission time: " << PROFILE_TIME("cmds") << " seconds");
	
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
				break;
			}
			case Event::KeyUp: {
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
	wci.width = 1280;
	wci.height = 720;
	wci.posX = 100;
	wci.posY = 100;
	wci.title = "Vulkan Engine";
	wci.borderless = true;

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

	assets.cleanup();
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
World Engine::world;
AssetStore Engine::assets;
float Engine::maxDepth;
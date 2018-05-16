#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include "Image.hpp"
#include "Profiler.hpp"
#include "PBRImage.hpp"
#include "OverlayRenderer.hpp"
#include "Threading.hpp"

/*
	@brief	Initialise enigne and sub-systems
*/
void Engine::start()
{
	validationWarning = false;
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

	rand.seed(0);
	maxDepth = 5000.f;

	/// This code joins separate albedo/spec and normal/rough images into a single image
	/* std::array<std::string, 7> tn = { "bamboo", "copper", "dirt", "greasymetal", "mahog", "marble", "rustediron" };

	std::string td = "res/textures/";

	for (int i = 0; i < tn.size(); ++i)
	{
		PBRImage c; c.create(Image(td + tn[i] + "_D.png"), Image(td + tn[i] + "_N.png"), Image(td + tn[i] + "_S.png"), Image(td + tn[i] + "_R.png"));
		c.albedoSpec.save(td + tn[i] + "_DS.png");
		c.normalRough.save(td + tn[i] + "_NR.png");
	} */

	camera.initialiseProj(float(window->resX) / float(window->resY), glm::pi<float>() / 2.f, 0.1, maxDepth);

	renderer = new Renderer();
	renderer->initialise();
	physicsWorld.create();

	scriptEnv.initChai();

	PROFILE_END("init");

	PROFILE_PREALLOC("setuprender");
	PROFILE_PREALLOC("physics");
	PROFILE_PREALLOC("submitrender");
	PROFILE_PREALLOC("scripts");

	threading = new Threading(4);

	PROFILE_START("assets");

	assets.loadAssets("/res/resources.xml");

	while (!threading->allJobsDone())
	{
		// Wait for all assets to load
	}

	PROFILE_END("assets");

	PROFILE_START("descsets");

	renderer->updateGBufferDescriptorSets();
	renderer->updateShadowDescriptorSets();
	renderer->updatePBRDescriptorSets();
	renderer->updateScreenDescriptorSets();
	
	PROFILE_END("descsets");

	PROFILE_START("world");

	// Adding models to the world
	{
		std::string materialList[6] = { "bamboo", "greasymetal", "marble", "dirt", "mahog", "copper" };

		for (int i = 0; i < 4; ++i)
		{
			world.addModelInstance("hollowbox", "hollowbox" + std::to_string(i));
			int s = 100;
			int sh = s / 2;
			glm::fvec3 pos = glm::fvec3(s64(rand() % s) - sh, s64(rand() % 1000) / 5.f + 50.f, s64(rand() % s) - sh);
			//world.modelNames["hollowbox" + std::to_string(i)]->transform = glm::translate(glm::fmat4(1), glm::fvec3(-((i % 5) * 2), std::floor(int(i / (int)5) * 2), 0));
			world.modelNames["hollowbox" + std::to_string(i)]->transform.setTranslation(pos);
			world.modelNames["hollowbox" + std::to_string(i)]->transform.setScale(glm::fvec3(10));
			world.modelNames["hollowbox" + std::to_string(i)]->transform.updateMatrix();
			world.modelNames["hollowbox" + std::to_string(i)]->setMaterial(assets.getMaterial(materialList[i % 6]));
			world.modelNames["hollowbox" + std::to_string(i)]->makePhysicsObject();
		}

		world.addModelInstance("pbrsphere", "pbrsphere");
		world.modelNames["pbrsphere"]->transform.setTranslation(glm::fvec3(4, 0, 0));
		world.modelNames["pbrsphere"]->transform.setScale(glm::fvec3(10));
		world.modelNames["pbrsphere"]->transform.updateMatrix();
		world.modelNames["pbrsphere"]->setMaterial(assets.getMaterial("marble"));
		world.modelNames["pbrsphere"]->makePhysicsObject();

		world.addModelInstance("ground", "ground");
		world.modelNames["ground"]->transform.setTranslation(glm::fvec3(0, 0, 0));
		world.modelNames["ground"]->transform.setScale(glm::fvec3(10));
		world.modelNames["ground"]->transform.updateMatrix();
		world.modelNames["ground"]->setMaterial(assets.getMaterial("dirt"));
		world.modelNames["ground"]->makePhysicsObject();

		//world.addModelInstance("monkey");
		//world.modelNames["monkey"]->transform = glm::translate(glm::fmat4(1), glm::fvec3(0, 10, 0));
	}

	while (!threading->allJobsDone())
	{
	}

	PROFILE_END("world");

	PROFILE_START("cmds");

	world.frustumCulling(&camera);
	renderer->updateTransformBuffer();
	renderer->populateDrawCmdBuffer();

	renderer->updateGBufferCommands();
	renderer->updateShadowCommands();
	renderer->updatePBRCommands();
	renderer->updateScreenCommands();
	renderer->overlayRenderer.updateOverlayCommands();

	PROFILE_END("cmds");

	std::cout << std::endl;
	DBG_INFO("Initialisation time     : " << PROFILE_TIME("init") << " seconds");
	DBG_INFO("Asset load time         : " << PROFILE_TIME("assets") << " seconds");
	DBG_INFO("Descriptor sets time    : " << PROFILE_TIME("descsets") << " seconds");
	DBG_INFO("World loading time      : " << PROFILE_TIME("world") << " seconds");
	DBG_INFO("Command submission time : " << PROFILE_TIME("cmds") << " seconds");
	
	console = new Console();
	console->create();

	Text* t = new Text;
	t->setName("stats");
	t->setFont(Engine::assets.getFont("consola"));
	t->setColour(glm::fvec4(0.2, 0.95, 0.2, 1));
	t->setCharSize(20);
	t->setString("");
	t->setPosition(glm::fvec2(0, 310));

	uiLayer = new OLayer;
	uiLayer->create(glm::ivec2(1280, 720));
	uiLayer->addElement(t);

	renderer->overlayRenderer.addLayer(uiLayer);
	renderer->overlayRenderer.addLayer(console->getLayer());

	scriptEnv.evalFile("./res/scripts/script.chai");

	std::function<void(void)> physicsJobFunc = []() -> void {
		PROFILE_START("physics");
		Engine::threading->physBulletMutex.lock();
		physicsWorld.step(Engine::frameTime);
		Engine::threading->physBulletMutex.unlock();
		PROFILE_END("physics");
	};

	std::function<void(void)> physicsJobDoneFunc = [&physicsJobFunc, &physicsJobDoneFunc]() -> void {
		Engine::threading->totalJobsFinished.fetch_add(1);
	};

	std::function<void(void)> physicsToEngineJobFunc = []() -> void {
		Engine::threading->physToEngineMutex.lock();
		Engine::threading->instanceTransformMutex.lock();
		physicsWorld.updateModels();
		Engine::threading->instanceTransformMutex.unlock();
		Engine::threading->physToEngineMutex.unlock();
	};

	std::function<void(void)> physicsToEngineJobDoneFunc = [&physicsToEngineJobFunc, &physicsToEngineJobDoneFunc]() -> void {
		Engine::threading->totalJobsFinished.fetch_add(1);
	};

	std::function<void(void)> physicsToGPUJobFunc = []() -> void {
		Engine::threading->physToGPUMutex.lock();
		Engine::renderer->updateTransformBuffer();
		Engine::threading->physToGPUMutex.unlock();
	};

	std::function<void(void)> physicsToGPUJobDoneFunc = [&physicsToGPUJobFunc, &physicsToGPUJobDoneFunc]() -> void {
		Engine::threading->totalJobsFinished.fetch_add(1);
	};

	std::function<void(void)> renderJobFunc = []() -> void {

		PROFILE_START("setuprender");

		Engine::threading->physToEngineMutex.lock();
		Engine::world.frustumCulling(&Engine::camera);
		Engine::renderer->populateDrawCmdBuffer(); // Mutex with engine model transform update
		Engine::threading->physToEngineMutex.unlock();

		Engine::renderer->overlayRenderer.updateOverlayCommands(); // Mutex with any overlay additions/removals
		Engine::renderer->updateShadowCommands(); // Mutex with engine model transform update
		Engine::renderer->updateGBufferCommands();
		Engine::renderer->updateCameraBuffer();

		PROFILE_END("setuprender");

		PROFILE_START("submitrender");

		Engine::threading->physToGPUMutex.lock();
		Engine::renderer->render();
		Engine::threading->physToGPUMutex.unlock();

		PROFILE_END("submitrender");
	};

	std::function<void(void)> renderJobDoneFunc = [&renderJobFunc, &renderJobDoneFunc]() -> void {
		Engine::threading->totalJobsFinished.fetch_add(1);
	};

	std::function<void(void)> scriptsJobFunc = []() -> void {
		PROFILE_START("scripts");
		Engine::threading->instanceTransformMutex.lock();
		try {
			Engine::scriptEnv.evalString("updateCamera()");
		}
		catch (chaiscript::exception::eval_error e)
		{
			std::cout << e.what() << std::endl;
		}
		Engine::threading->instanceTransformMutex.unlock();
		PROFILE_END("scripts");
	};

	std::function<void(void)> scriptsJobDoneFunc = []() -> void {
		Engine::threading->totalJobsFinished.fetch_add(1);
	};

	threading->addJob(new Job<>(physicsJobFunc, physicsJobDoneFunc));
	threading->addJob(new Job<>(physicsToEngineJobFunc, physicsToEngineJobDoneFunc));
	threading->addJob(new Job<>(physicsToGPUJobFunc, physicsToGPUJobDoneFunc));
	threading->addJob(new Job<>(renderJobFunc, renderJobDoneFunc));
	threading->addJob(new Job<>(scriptsJobFunc, scriptsJobDoneFunc));

	Time frameStart; frameStart = engineStartTime;
	frameTime.setMicroSeconds(0);
	double fpsDisplay = 0.f;
	int frames = 0;
	std::vector<float> times;
	while (engineRunning)
	{
		PROFILE_START("msgevent");

		while (window->processMessages()) { /* Invoke timer ? */ }
		
#ifdef _WIN32
		GetKeyboardState(Keyboard::keyState);
#endif

		Event ev;
		while (window->eventQ.pollEvent(ev)) {
			/*try {
				scriptEnv.evalString("processEvent()");
			}
			catch (chaiscript::exception::eval_error e)
			{
				std::cout << e.what() << std::endl;
			}*/
			switch (ev.type) {
			case(Event::TextInput):
			{
				console->inputChar(ev.eventUnion.textInputEvent.character);
				break;
			}
			case(Event::MouseWheel):
			{
				console->scroll(ev.eventUnion.mouseEvent.wheelDelta);
				break;
			}
			case(Event::MouseDown):
			{
				if (ev.eventUnion.mouseEvent.code & Mouse::M_LEFT)
				{
					Engine::threading->physBulletMutex.lock(); /// TODO: this avoids a crash, but we dont want to hang the main thread for adding a constraint !!! maybe this should be a job, or better, batch all physics changes in one job
					glm::fvec3 p = camera.getPosition();
					btVector3 camPos(p.x, p.y, p.z);

					btVector3 rayFrom = camPos;
					btVector3 rayTo = physicsWorld.getRayTo(int(ev.eventUnion.mouseEvent.position.x), int(ev.eventUnion.mouseEvent.position.y));

					physicsWorld.pickBody(rayFrom, rayTo);
					Engine::threading->physBulletMutex.unlock();
				}
				break;
			}
			case(Event::MouseUp):
			{
				if (ev.eventUnion.mouseEvent.code & Mouse::M_LEFT)
				{
					Engine::threading->physBulletMutex.lock(); /// TODO: this avoids a crash, but we dont want to hang the main thread for adding a constraint !!! maybe this should be a job, or better, batch all physics changes in one job
					physicsWorld.removePickingConstraint();
					Engine::threading->physBulletMutex.unlock();
				}
				break;
			}
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
				auto key = ev.eventUnion.keyEvent.key.code;
				if (console->isActive())
					console->moveBlinker(key);

				if (key == Key::KC_ESCAPE)
				{
					if (console->isActive())
						console->toggle();
					else
						engineRunning = false;
				}
				if (key == Key::KC_TILDE)
				{
					console->toggle();
				}
				break;
			}
			case Event::KeyUp: {
				break;
			}
			}
			window->eventQ.popEvent();
		}
		physicsWorld.mouseMoveCallback(Mouse::getWindowPosition(window).x, Mouse::getWindowPosition(window).y);

		console->update();

		PROFILE_END("msgevent");

		if (threading->allJobsDone())
		{
			frameTime = clock.time() - frameStart;
			frameStart = clock.time();

			threading->cleanupJobs();

			threading->addJob(new Job<>(physicsJobFunc, physicsJobDoneFunc));
			threading->addJob(new Job<>(physicsToEngineJobFunc, physicsToEngineJobDoneFunc));
			threading->addJob(new Job<>(physicsToGPUJobFunc, physicsToGPUJobDoneFunc));
			threading->addJob(new Job<>(renderJobFunc, renderJobDoneFunc));
			threading->addJob(new Job<>(scriptsJobFunc, scriptsJobDoneFunc));

			// FPS display
			++frames;
			fpsDisplay += frameTime.getSeconds();
			if (fpsDisplay > 0.75f)
			{
				double gBufferTime = double(Engine::gpuTimeStamps[Renderer::END_GBUFFER] - Engine::gpuTimeStamps[Renderer::BEGIN_GBUFFER]) * 0.000001;
				double shadowTime = double(Engine::gpuTimeStamps[Renderer::END_SHADOW] - Engine::gpuTimeStamps[Renderer::BEGIN_SHADOW]) * 0.000001;
				double pbrTime = double(Engine::gpuTimeStamps[Renderer::END_PBR] - Engine::gpuTimeStamps[Renderer::BEGIN_PBR]) * 0.000001;
				double overlayTime = double(Engine::gpuTimeStamps[Renderer::END_OVERLAY] - Engine::gpuTimeStamps[Renderer::BEGIN_OVERLAY]) * 0.000001;
				double overlayCombineTime = double(Engine::gpuTimeStamps[Renderer::END_OVERLAY_COMBINE] - Engine::gpuTimeStamps[Renderer::BEGIN_OVERLAY_COMBINE]) * 0.000001;
				double screenTime = double(Engine::gpuTimeStamps[Renderer::END_SCREEN] - Engine::gpuTimeStamps[Renderer::BEGIN_SCREEN]) * 0.000001;

				double totalGPUTime = gBufferTime + shadowTime + pbrTime + overlayTime + overlayCombineTime + screenTime;

				auto stats = (Text*)uiLayer->getElement("stats");

				stats->setString(
					"---------------------------\n"
					"GBuffer pass   : " + std::to_string(gBufferTime) + "ms\n" +
					"Shadow pass    : " + std::to_string(shadowTime) + "ms\n" +
					"PBR pass       : " + std::to_string(pbrTime) + "ms\n" +
					"Overlay pass   : " + std::to_string(overlayTime) + "ms\n" +
					"OCombine pass  : " + std::to_string(overlayCombineTime) + "ms\n" +
					"Screen pass    : " + std::to_string(screenTime) + "ms\n\n" +

					"Total GPU time : " + std::to_string(totalGPUTime) + "ms\n" +
					"GPU FPS        : " + std::to_string((int)(1.0 / (totalGPUTime*0.001))) + "\n\n" +

					"---------------------------\n" +
					"User input     : " + std::to_string(PROFILE_TIME_MS("msgevent")) + "ms\n" +
					"Set up render  : " + std::to_string(PROFILE_TIME_MS("setuprender")) + "ms\n" +
					"Submit render  : " + std::to_string(PROFILE_TIME_MS("submitrender") - totalGPUTime) + "ms\n" +
					"Physics        : " + std::to_string(PROFILE_TIME_MS("physics")) + "ms\n" +
					"Scripts        : " + std::to_string(PROFILE_TIME_MS("scripts")) + "ms\n\n" +

					"Avg frame time : " + std::to_string((fpsDisplay * 1000) / double(frames)) + "ms\n" +
					"FPS            : " + std::to_string((int)(double(frames) / fpsDisplay))
				);
				fpsDisplay = 0.f;
				frames = 0;
			}
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

	VK_CHECK_RESULT(vkCreateInstance(&instInfo, nullptr, &vkInstance));
	
#ifdef ENABLE_VULKAN_VALIDATION
	DBG_INFO("Creating validation layer debug callback");
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallbackFunc;

	auto createDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugReportCallbackEXT");

	VK_CHECK_RESULT(createDebugReportCallbackEXT(vkInstance, &createInfo, nullptr, &debugCallbackInfo));
#endif
}

/*
	@brief	Queries vulkan instance for physical device information, picks best one
*/
void Engine::queryVulkanPhysicalDeviceDetails()
{
	DBG_INFO("Picking Vulkan device");
		uint32_t deviceCount = 0;
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr));

	if (deviceCount == 0)
		DBG_SEVERE("No GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data()));

	physicalDevicesDetails.clear();

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
	VK_VALIDATE(vkDestroyInstance(vkInstance, nullptr));
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
	validationWarning = true;
	validationMessage = msg;
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
PhysicsWorld Engine::physicsWorld;
VkPhysicalDevice Engine::vkPhysicalDevice = VK_NULL_HANDLE;
std::vector<PhysicalDeviceDetails> Engine::physicalDevicesDetails;
int Engine::physicalDeviceIndex;
bool Engine::engineRunning = true;
Time Engine::engineStartTime;
Camera Engine::camera;
World Engine::world;
AssetStore Engine::assets;
float Engine::maxDepth;
std::mt19937_64 Engine::rand;
u64 Engine::gpuTimeStamps[Renderer::NUM_GPU_TIMESTAMPS];
bool Engine::validationWarning;
std::string Engine::validationMessage;
OLayer* Engine::uiLayer;
Console* Engine::console;
Time Engine::frameTime;
ScriptEnv Engine::scriptEnv;
Threading* Engine::threading;
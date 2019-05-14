#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include "Image.hpp"
#include "Profiler.hpp"
#include "PBRImage.hpp"
#include "UIRenderer.hpp"
#include "Threading.hpp"

#include "Filesystem.hpp"

/*
	@brief	Initialise enigne and sub-systems and start main loop
*/
void Engine::start()
{
	DBG_INFO("Launching engine");
	engineStartTime = clock.time();
#ifdef _WIN32
	{
		char cwd[_MAX_DIR];
		GetCurrentDir(cwd, _MAX_DIR);
		workingDirectory.assign(cwd).append("/");
		DBG_INFO("Working Directory: " << workingDirectory);
	}
#endif

	assets.gatherAvailableAssets();

	/*
		Create basic engine commponents
	*/
	createVulkanInstance();
	createWindow();
	queryVulkanPhysicalDeviceDetails();
	rand.seed(0);

	/// TODO: A more convenient utility for this \/ \/ \/
	/// This code joins separate albedo(3 byte)/spec(1 byte) and normal(3 byte)/rough(1 byte) images into single(4 byte) images
	/* std::array<std::string, 7> tn = { "bamboo", "copper", "dirt", "greasymetal", "mahog", "marble", "rustediron" };

	std::string td = "res/textures/";

	for (int i = 0; i < tn.size(); ++i)
	{
		PBRImage c; c.create(Image(td + tn[i] + "_D.png"), Image(td + tn[i] + "_N.png"), Image(td + tn[i] + "_S.png"), Image(td + tn[i] + "_R.png"));
		c.albedoSpec.save(td + tn[i] + "_DS.png");
		c.normalRough.save(td + tn[i] + "_NR.png");
	} */

	/// TODO: temporary, we need a configurations manager
	maxDepth = 1000000.f;

	/// TODO: cameras should be objects in the world, need an entity component system
	camera.initialiseProj(float(window->resX) / float(window->resY), glm::radians(90.f), 0.1, maxDepth);
	camera.setPosition(glm::fvec3(5, 5, 5));
	camera.update();


	/*
		Initialise vulkan logical device
	*/
	renderer = new Renderer();
	renderer->initialiseDevice();
	renderer->logicalDevice.setVkErrorCallback(&vduVkErrorCallback);
	renderer->logicalDevice.setVduDebugCallback(&vduDebugCallback);

	/*
		Initialise worker threads (each one needs vulkan logical device before initialising its command pool)
	*/
	int numThreads = 0; // Extra threads to the 4 compulsory ones (MAIN thread, CPU thread, GPU submission thread, DISK IO thread)
	waitForProfilerInitMutex.lock();
	threading = new Threading(numThreads);

	/*
		Preallocating profiler tags to avoid thread clashes
	*/
	std::vector<std::string> profilerTags = { 
		"init", "setuprender", "physics", "submitrender", "scripts", "qwaitidle", // CPU Tags
		"shadowfence", "gbufferfence",

		"gbuffer", "shadow", "pbr", "overlay", "screen", "commands", "cullingdrawbuffer", // GPU Tags

		"physmutex", "phystoenginemutex", "phystogpumutex", // Mutex tags
		"transformmutex", "modeladdmutex"
	};

	int i = 0;
	std::vector<std::thread::id> threadIDs(numThreads+4);
	threadIDs[i] = std::this_thread::get_id(); // Main thread can use profiler
	++i;
	for (auto& thread : threading->m_workerThreads) {
		threadIDs[i] = thread->getID();
		threading->m_threadIDAssociations.insert(std::make_pair(i, thread->getID()));
		++i;
	}
	
	Profiler::prealloc(threadIDs, profilerTags);
	waitForProfilerInitMutex.unlock();

	/*
		Initialise scripting environment and initialise configs from script 'config.chai'
		Camera movement is done within a script right now
	*/
	scriptEnv.initChai();
	scriptEnv.chai.add_global(chaiscript::var(std::ref(world)), "world");
	scriptEnv.chai.add_global(chaiscript::var(std::ref(assets)), "assets");
	scriptEnv.chai.add_global(chaiscript::var(std::ref(config)), "config");
	scriptEnv.evalFile("./res/scripts/config.chai");

	/*
		Initialise the rest of the renderer resources (now that threads are initialised we can submit jobs)
	*/
	auto initRendererJobFunc = []() -> void {
		renderer->initialise();
	};
	threading->addGPUJob(new Job<>(initRendererJobFunc));
	renderer->createPerThreadCommandPools();

	/*
		Wait until the renderer is initialised before we start loading assets
	*/
	threading->m_gpuWorker->waitForAllJobsToFinish();

	/*
		Default assets are null assets (blank textures to fall back on when desired not found)
		Also load a font for the console to use for reporting initialisation progress
		We need to block execution until they are loaded so we dont have any threading errors and the console can work
	*/
	assets.loadDefaultAssets();
	assets.loadAssets("/res/consolefontresource.xml");
	world.addModelInstance("nullModel", "nullModel"); // Null model needed in the world to prevent warning about empty vertex/index buffers

	threading->m_gpuWorker->waitForAllJobsToFinish();

	/*
		The console will start reporting progress of initialisation (loading assets)
	*/
	console = new Console();
	console->create(glm::ivec2(window->resX, 276));
	//renderer->uiRenderer.createUIGroup(console->getUIGroup()); // The console is a UI overlay
	scriptEnv.chai.add_global(chaiscript::var(std::ref(*console)), "console"); // Add console to script environment

	/*
		This graphics job will 'loop' (see Console::renderAtStartup) until initialisation is complete
	*/
	auto renderConsoleFunc = []() -> void {
		renderer->uiRenderer.updateOverlayCommands();
		renderer->updateScreenDescriptorSets();
		renderer->updateScreenCommandsForConsole();
		console->renderAtStartup();
	};

	/*
		Submit graphics job (console rendering)
	*/
	auto renderConsoleJob = new Job<>(renderConsoleFunc);
	threading->addGPUJob(renderConsoleJob);

	/*
		Create bullet physics dynamic world
	*/
	physicsWorld.create();

	/*
		Camera movement done in this script
	*/
	scriptEnv.evalFile("./res/scripts/script.chai");

	/*
		Load the rest of our assets defined in 'resources.xml'
	*/
	assets.loadAssets("/res/resources.xml");
	
	
	/*
		Update descriptor sets for all pipelines
	*/

	renderer->updateGBufferDescriptorSets();
	renderer->updateGBufferNoTexDescriptorSets();
	renderer->updateShadowDescriptorSets();
	renderer->updatePBRDescriptorSets(renderer->usedGBuffer);
	renderer->updateSSAODescriptorSets();

	initialised = 1;

	// Load various assets
	assets.loadAsset(Asset::Model, "simplepillar2");
	assets.loadAsset(Asset::Model, "simplepillar");
	assets.loadAsset(Asset::Model, "arena");
	assets.loadAsset(Asset::Model, "simpledoor");
	assets.loadAsset(Asset::Model, "PBRCube");
	assets.loadAsset(Asset::Model, "ground");

	threading->m_gpuWorker->waitForAllJobsToFinish();

	// Startup script. Adds models to the world
	scriptEnv.evalFile("./res/scripts/startup.chai");
	

	/*
		Creating UI layer for displaying profiling stats
	*/

	uiGroup = renderer->uiRenderer.createUIGroup("stats");
	uiGroup->setResolution(glm::ivec2(1280, 720));

	// For GPU and CPU profiling times
	auto statsText = uiGroup->createElement<Text>("stats");
	statsText->setFont(Engine::assets.getFont("consola"));
	statsText->setColour(glm::fvec4(0.2, 0.95, 0.2, 1));
	statsText->setCharSize(20);
	statsText->setString("");
	statsText->setPosition(glm::fvec2(0, 330));

	// For mutex lock wait times
	auto mutexStatsText = uiGroup->createElement<Text>("mutexstats");
	mutexStatsText->setFont(Engine::assets.getFont("consola"));
	mutexStatsText->setColour(glm::fvec4(0.2, 0.95, 0.2, 1));
	mutexStatsText->setCharSize(20);
	mutexStatsText->setString("");
	mutexStatsText->setPosition(glm::fvec2(600, 450));

	// For per thread stats
	auto threadStatsText = uiGroup->createElement<Text>("threadstats");
	threadStatsText->setFont(Engine::assets.getFont("consola"));
	threadStatsText->setColour(glm::fvec4(0.2, 0.95, 0.2, 1));
	threadStatsText->setCharSize(20);
	threadStatsText->setString("");
	threadStatsText->setPosition(glm::fvec2(600, 590));

	// GPU commands
	{
		console->getStartupRenderFence().wait();
		renderer->updateScreenCommands();
		renderer->updatePBRCommands();
		renderer->updateGBufferCommands();
		renderer->updateGBufferNoTexCommands();
		renderer->updateShadowCommands(); // Mutex with engine model transform update
		renderer->updateSSAOCommands();
		renderer->uiRenderer.updateOverlayCommands(); // Mutex with any overlay additions/removals
	}

	/*
		These jobs push themselves back into the job queue while Engine::engineRunning == true
	*/
	threading->addGPUJob(new Job<>(&Renderer::renderJob));
	threading->addCPUJob(new Job<>(&PhysicsWorld::updateJob));
	threading->addCPUJob(new Job<>(&Threading::cleanupJob));

	world.setSkybox("skybox");

	engineLoop();
}

void Engine::engineLoop()
{
	// Used to measure frame time
	Time frameStart; frameStart = engineStartTime;

	// Main engine loop
	while (engineRunning)
	{
		PROFILE_START("thread_" + Threading::getThisThreadIDString());

		frameTime = clock.time() - frameStart;
		frameStart = clock.time();

		processNextMainThreadJob();

		// Process OS messages (user input, etc) and translate them to engine events
		PROFILE_START("msgevent");
		window->processMessages();
		PROFILE_END("msgevent");

#ifdef _WIN32
		// We get keyboard state here for SHIFT+KEY events to work properly
		GetKeyboardState(os::Keyboard::keyState);
#endif

		eventLoop();

		// Script game tick
		PROFILE_START("scripts");
		PROFILE_MUTEX("transformmutex", Engine::threading->instanceTransformMutex.lock());
		try {
			Engine::scriptEnv.evalString("gameTick()");
		}
		catch (chaiscript::exception::eval_error e)
		{
			console->postMessage(e.what(), glm::fvec3(1.0, 0.05, 0.05)); // Report any script evaluation errors to the console
			console->setActive(true);
		}
		Engine::threading->instanceTransformMutex.unlock();
		PROFILE_END("scripts");

		console->update();

		// Display performance stats
		timeSinceLastStatsUpdate += frameTime.getSeconds();
		if (timeSinceLastStatsUpdate > 0.05f)
		{
			updatePerformanceStatsDisplay();
			timeSinceLastStatsUpdate = 0.f;
		}

		PROFILE_END("thread_" + Threading::getThisThreadIDString());
	}

	quit();
}

void Engine::eventLoop()
{
	// Process engine events
	Event ev;
	window->eventQ.pushEventMutex.lock();
	while (window->eventQ.pollEvent(ev)) {
		switch (ev.type) {
		case(Event::WindowResized):
		{
			auto consoleSizeUpdateJobFunc = std::bind([](Console* cons) -> void {
				renderer->gBufferGroupFence.wait();
				console->setResolution(glm::ivec2(Engine::window->resX, 276));
				console->postMessage("Window resized", glm::fvec3(0.2, 0.9, 0.2));
			}, console);
			threading->addGPUJob(new Job<>(consoleSizeUpdateJobFunc));
		}
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
			if (ev.eventUnion.mouseEvent.code & os::Mouse::M_LEFT)
			{
				PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock()); /// TODO: this avoids a crash, but we dont want to hang the main thread for adding a constraint !!! maybe this should be a job, or better, batch all physics changes in one job
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
			if (ev.eventUnion.mouseEvent.code & os::Mouse::M_LEFT)
			{
				PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock()); /// TODO: this avoids a crash, but we dont want to hang the main thread for adding a constraint !!! maybe this should be a job, or better, batch all physics changes in one job
				physicsWorld.removePickingConstraint();
				Engine::threading->physBulletMutex.unlock();
			}
			break;
		}
		case(Event::MouseMove):
		{
			if (ev.eventUnion.mouseEvent.code & os::Mouse::M_RIGHT)
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

			if (key == os::Key::KC_ESCAPE)
			{
				if (console->isActive())
					console->toggle();
				else
					engineRunning = false;
			}
			if (console->isActive())
				break;
			if (key == os::Key::KC_TILDE)
			{
				console->toggle();
			}
			if (key == os::Key::KC_L)
			{
				auto reloadJobFunc = []() -> void {
					Engine::renderer->reloadShaders();
				};
				threading->addGPUJob(new Job<>(reloadJobFunc));
			}
			break;
		}
		case Event::KeyUp: {
			break;
		}
		}
		window->eventQ.popEvent();
	}
	physicsWorld.mouseMoveCallback(os::Mouse::getWindowPosition(window).x, os::Mouse::getWindowPosition(window).y);
	window->eventQ.pushEventMutex.unlock();
}

/*
	Main thread can steal CPU jobs from the CPU worker thread
*/
void Engine::processNextMainThreadJob()
{
	renderer->executeFenceDelayedActions(); // Any externally synced vulkan objects created on this thread should be destroyed from this thread
	JobBase* job;
	if (threading->m_cpuWorker->popJob(job)) {
		job->run();
	}
}

void Engine::updatePerformanceStatsDisplay()
{
	auto gBufferTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("gbuffer"));
	auto shadowTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("shadow"));
	auto ssaoTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("ssao"));
	auto pbrTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("pbr"));
	auto overlayTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("overlay"));
	auto screenTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("screen"));

	auto gBufferTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("gbuffer"));
	auto shadowTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("shadow"));
	auto ssaoTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("ssao"));
	auto pbrTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("pbr"));
	auto overlayTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("overlay"));
	auto screenTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("screen"));

	auto gBufferTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("gbuffer"));
	auto shadowTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("shadow"));
	auto ssaoTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("ssao"));
	auto pbrTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("pbr"));
	auto overlayTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("overlay"));
	auto screenTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("screen"));

	auto totalGPUTime = gBufferTime + ssaoTime + shadowTime + pbrTime + overlayTime + screenTime;
	auto totalGPUMaxTime = gBufferTimeMax + ssaoTimeMax + shadowTimeMax + pbrTimeMax + overlayTimeMax + screenTimeMax;
	auto totalGPUMinTime = gBufferTimeMin + ssaoTimeMax + shadowTimeMin + pbrTimeMin + overlayTimeMin + screenTimeMin;

	auto gpuFPS = 1000.0 / totalGPUTime;
	auto gpuFPSMin = 1000.0 / totalGPUMinTime;
	auto gpuFPSMax = 1000.0 / totalGPUMaxTime;

	auto msgTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("msgevent"));
	auto cullTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("cullingdrawbuffer"));
	auto cmdsTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("commands"));
	auto submitTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("submitrender"));
	auto qWaitTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("qwaitidle"));
	auto physicsTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("physics"));
	auto scriptsTime = PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("scripts"));

	auto msgTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("msgevent"));
	auto cullTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("cullingdrawbuffer"));
	auto cmdsTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("commands"));
	auto submitTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("submitrender"));
	auto qWaitTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("qwaitidle"));
	auto physicsTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("physics"));
	auto scriptsTimeMax = PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("scripts"));

	auto msgTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("msgevent"));
	auto cullTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("cullingdrawbuffer"));
	auto cmdsTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("commands"));
	auto submitTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("submitrender"));
	auto qWaitTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("qwaitidle"));
	auto physicsTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("physics"));
	auto scriptsTimeMin = PROFILE_TO_MS(PROFILE_GET_RUNNING_MIN("scripts"));

	auto stats = (Text*)uiGroup->getElement("stats");

	stats->setString(
		"---------------------------------------------------\n"
		"GBuffer pass   : " + std::to_string(gBufferTime) + "ms ( " + std::to_string(gBufferTimeMin) + " \\ " + std::to_string(gBufferTimeMax) + " )\n" +
		"Shadow pass    : " + std::to_string(shadowTime) + "ms ( " + std::to_string(shadowTimeMin) + " \\ " + std::to_string(shadowTimeMax) + " )\n" +
		"SSAO pass      : " + std::to_string(ssaoTime) + "ms ( " + std::to_string(ssaoTimeMin) + " \\ " + std::to_string(ssaoTimeMax) + ")\n" +
		"PBR pass       : " + std::to_string(pbrTime) + "ms ( " + std::to_string(pbrTimeMin) + " \\ " + std::to_string(pbrTimeMax) + " )\n" +
		"Overlay pass   : " + std::to_string(overlayTime) + "ms ( " + std::to_string(overlayTimeMin) + " \\ " + std::to_string(overlayTimeMax) + " )\n" +
		"Screen pass    : " + std::to_string(screenTime) + "ms ( " + std::to_string(screenTimeMin) + " \\ " + std::to_string(screenTimeMax) + " )\n" +

		"Total GPU time : " + std::to_string(totalGPUTime) + "ms ( " + std::to_string(totalGPUMinTime) + " \\ " + std::to_string(totalGPUMaxTime) + " )\n" +
		"GPU FPS        : " + std::to_string((int)gpuFPS) + " ( " + std::to_string((int)gpuFPSMax) + " \\ " + std::to_string((int)gpuFPSMin) + " )\n\n" +

		"---------------------------------------------------\n" +
		"User input     : " + std::to_string(msgTime) + "ms ( " + std::to_string(msgTimeMin) + " \\ " + std::to_string(msgTimeMax) + " )\n" +
		"Commands       : " + std::to_string(cmdsTime) + "ms ( " + std::to_string(cmdsTimeMin) + " \\ " + std::to_string(cmdsTimeMax) + " )\n" +
		"Culling & draw : " + std::to_string(cullTime) + "ms ( " + std::to_string(cullTimeMin) + " \\ " + std::to_string(cullTimeMax) + " )\n" +
		"Queue submit   : " + std::to_string(submitTime) + "ms ( " + std::to_string(submitTimeMin) + " \\ " + std::to_string(submitTimeMax) + " )\n" +
		"Queue idle     : " + std::to_string(qWaitTime) + "ms ( " + std::to_string(qWaitTimeMin) + " \\ " + std::to_string(qWaitTimeMax) + " )\n" +
		"Physics        : " + std::to_string(physicsTime) + "ms ( " + std::to_string(physicsTimeMin) + " \\ " + std::to_string(physicsTimeMax) + " )\n" +
		"Scripts        : " + std::to_string(scriptsTime) + "ms ( " + std::to_string(scriptsTimeMin) + " \\ " + std::to_string(scriptsTimeMax) + " )\n\n"// +

		//"Avg frame time : " + std::to_string((timeSinceLastStatsUpdate * 1000) / double(frames)) + "ms\n" +
		//"FPS            : " + std::to_string((int)(double(frames) / timeSinceLastStatsUpdate))
	);

	auto mutexStats = (Text*)uiGroup->getElement("mutexstats");

	mutexStats->setString(
		"----MUTEXES----------------\n"
		"---------------------------\n"
		"Physics        : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("physmutex"))) + "ms\n" +
		"Phys->Engine   : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("phystoenginemutex"))) + "ms\n" +
		"Phys->GPU      : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("phystogpumutex"))) + "ms\n" +
		"Transform      : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("transformmutex"))) + "ms\n"
	);

	auto threadStats = (Text*)uiGroup->getElement("threadstats");

	std::string threadStatsString;

	threadStatsString = "----THREADS-------------------\n------------------------------\n"
		"Thread_1 (main)    : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("thread_" + Threading::getThreadIDString(std::this_thread::get_id())))) + "ms ( " + std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("thread_" + Threading::getThreadIDString(std::this_thread::get_id())))) + " )\n";

	threadStatsString += std::string("Thread_2 (cpu)  : " +
		std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("thread_" + Threading::getThreadIDString(threading->m_threadIDAssociations[1])))) + "ms ( " + 
		std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("thread_" + Threading::getThreadIDString(threading->m_threadIDAssociations[1]))))) + " )\n";

	threadStatsString += std::string("Thread_3 (gpu)    : " +
		std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("thread_" + Threading::getThreadIDString(threading->m_threadIDAssociations[2])))) + "ms ( " +
		std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("thread_" + Threading::getThreadIDString(threading->m_threadIDAssociations[2]))))) + " )\n";

	threadStatsString += std::string("Thread_3 (disk)    : " +
		std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("thread_" + Threading::getThreadIDString(threading->m_threadIDAssociations[3])))) + "ms ( " +
		std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("thread_" + Threading::getThreadIDString(threading->m_threadIDAssociations[3]))))) + " )\n";

	for (int i = 3; i < threading->m_workerThreads.size(); ++i)
	{
		threadStatsString += std::string("Thread_") + std::to_string(i+3) + " (generic) : " +
			std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_AVERAGE("thread_" + Threading::getThreadIDString(threading->m_threadIDAssociations[i + 1])))) + "ms ( " +
			std::to_string(PROFILE_TO_MS(PROFILE_GET_RUNNING_MAX("thread_" + Threading::getThreadIDString(threading->m_threadIDAssociations[i + 1])))) + " )\n";
		++i;
	}

	threadStats->setString(threadStatsString);
}

void Engine::vduVkDebugCallback(vdu::Instance::DebugReportLevel level, vdu::Instance::DebugObjectType objectType, uint64_t objectHandle, const std::string & objectName, const std::string & message)
{
	std::string levelMsg;
	switch (level) {
	case vdu::Instance::DebugReportLevel::Warning:
		levelMsg = "Warning - ";
		break;
	case vdu::Instance::DebugReportLevel::Error:
		levelMsg = "Error   - ";
		break;
	}
	std::cout << levelMsg << "Object [" << objectName << "]\n" << message << "\n\n";
}

void Engine::vduVkErrorCallback(VkResult result, const std::string & message)
{
	DBG_SEVERE(message);
}

void Engine::vduDebugCallback(vdu::LogicalDevice::VduDebugLevel level, const std::string & message)
{
	switch (level) {
	case vdu::LogicalDevice::VduDebugLevel::Error:
		DBG_SEVERE(message);
		break;
	case vdu::LogicalDevice::VduDebugLevel::Warning:
		DBG_WARNING(message);
		break;
	case vdu::LogicalDevice::VduDebugLevel::Info:
		DBG_INFO(message);
		break;
	}
}

/*
	@brief	Create a window for drawing to
*/
void Engine::createWindow()
{
	DBG_INFO("Creating window");
	// Window creator requires OS specific handles

	os::WindowCreateInfo wci;
	wci.width = 1280;
	wci.height = 720;
	wci.posX = 0;
	wci.posY = 0;
	wci.title = "Vulkan Engine";
	wci.borderless = true;

	window = new os::Window;
	window->create(&wci);
}

/*
	@brief	Creates vulkan instance, enables extensions
	@note	If ENABLE_VULKAN_VALIDATION is defined, adds lunarg validation layer 
*/
void Engine::createVulkanInstance()
{
	DBG_INFO("Creating vulkan instance");

	vulkanInstance.setApplicationName("App");
	vulkanInstance.setEngineName("Engine");
	vulkanInstance.addExtension(VK_KHR_SURFACE_EXTENSION_NAME);
	vulkanInstance.addExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	vulkanInstance.addExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#ifdef ENABLE_VULKAN_VALIDATION
	vulkanInstance.addDebugReportLevel(vdu::Instance::DebugReportLevel::Warning);
	vulkanInstance.addDebugReportLevel(vdu::Instance::DebugReportLevel::Error);
	vulkanInstance.setDebugCallback(&vduVkDebugCallback);
	vulkanInstance.addLayer("VK_LAYER_LUNARG_standard_validation");
#endif

	VK_CHECK_RESULT(vulkanInstance.create());
}

/*
	@brief	Queries vulkan instance for physical device information, picks best one
*/
void Engine::queryVulkanPhysicalDeviceDetails()
{
	physicalDevice = &vulkanInstance.enumratePhysicalDevices().front();
	VK_CHECK_RESULT(physicalDevice->querySurfaceCapabilities(window->vkSurface));
}

/*
	@brief	Cleanup memory on exit
*/
void Engine::quit()
{
	DBG_INFO("Exiting");
	for (auto t : threading->m_workerThreads)
	{
		if (t)
			t->join();
	}

	threading->freeFinishedJobs(); // Cleanup memory of finished jobs
	renderer->lGraphicsQueue.waitIdle();
	renderer->lTransferQueue.waitIdle();
	assets.cleanup();
	console->cleanup();
	renderer->cleanup();
	window->destroy();
	vulkanInstance.destroy();
	delete renderer;
	delete window;
}

EngineConfig Engine::config;
Filesystem Engine::fs;
Clock Engine::clock;
os::Window* Engine::window;
Renderer* Engine::renderer;
PhysicsWorld Engine::physicsWorld;
bool Engine::engineRunning = true;
Time Engine::engineStartTime;
Time Engine::scriptTickTime;
Camera Engine::camera;
World Engine::world;
AssetStore Engine::assets;
float Engine::maxDepth;
std::mt19937_64 Engine::rand;
u64 Engine::gpuTimeStamps[Renderer::NUM_GPU_TIMESTAMPS];
UIElementGroup* Engine::uiGroup;
Console* Engine::console;
Time Engine::frameTime(0);
ScriptEnv Engine::scriptEnv;
Threading* Engine::threading;
std::atomic_char Engine::initialised = 0;
std::mutex Engine::waitForProfilerInitMutex;
double Engine::timeSinceLastStatsUpdate = 0.f;
vdu::Instance Engine::vulkanInstance;
vdu::PhysicalDevice* Engine::physicalDevice;
std::vector<vdu::PhysicalDevice> Engine::allPhysicalDevices;
std::string Engine::workingDirectory;
#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include "Image.hpp"
#include "Profiler.hpp"
#include "PBRImage.hpp"
#include "OverlayRenderer.hpp"
#include "Threading.hpp"

#include "CommonJobs.hpp"

/*
	@brief	Initialise enigne and sub-systems and start main loop
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

	/*
		Create basic engine commponents
	*/
	createVulkanInstance();
	createWindow();
	queryVulkanPhysicalDeviceDetails();
	initialiseCommonJobs();
	rand.seed(0);

	/// TODO: A more convenient utility for this \/ \/ \/
	/// This code joins separate albedo/spec and normal/rough images into a single image
	/* std::array<std::string, 7> tn = { "bamboo", "copper", "dirt", "greasymetal", "mahog", "marble", "rustediron" };

	std::string td = "res/textures/";

	for (int i = 0; i < tn.size(); ++i)
	{
		PBRImage c; c.create(Image(td + tn[i] + "_D.png"), Image(td + tn[i] + "_N.png"), Image(td + tn[i] + "_S.png"), Image(td + tn[i] + "_R.png"));
		c.albedoSpec.save(td + tn[i] + "_DS.png");
		c.normalRough.save(td + tn[i] + "_NR.png");
	} */

	/// TODO: temporary, we need a configurations manager
	maxDepth = 10000.f; 

	/// TODO: cameras should be objects in the world
	camera.initialiseProj(float(window->resX) / float(window->resY), glm::radians(120.f), 0.1, maxDepth);
	camera.setPosition(glm::fvec3(50, 50, 50));
	camera.update();

	/*
		Initialise vulkan logical device
	*/
	renderer = new Renderer();
	renderer->initialiseDevice();

	/*
		Initialise worker threads (each one needs vulkan logical device before initialising its command pool)
	*/
	int numThreads = 4;
	waitForProfilerInitMutex.lock();
	threading = new Threading(numThreads);

	/*
		Preallocating profiler tags to avoid thread clashes
	*/
	std::vector<std::string> profilerTags = { "init", "assets", "world", "setuprender", "physics", "submitrender", "scripts", "qwaitidle", // CPU Tags
		"shadowfence", "gbufferfence",

		"gbuffer", "shadow", "pbr", "overlay", "overlaycombine",  "screen", "commands", "cullingdrawbuffer", // GPU Tags

		"physmutex", "phystoenginemutex", "phystogpumutex", // Mutex tags
		"transformmutex", "modeladdmutex"
	};

	int i = 0;
	std::vector<std::thread::id> threadIDs(numThreads);
	for (auto& thread : threading->workers) {
		threadIDs[i] = thread->get_id();
		threading->threadIDAssociations.insert(std::make_pair(i + 1, thread->get_id()));
		++i;
	}
	threadIDs[i] = std::this_thread::get_id(); // Main thread can also use profiler

	Profiler::prealloc(threadIDs, profilerTags);
	waitForProfilerInitMutex.unlock();

	/*
		Initialise the rest of the renderer resources (now that threads are initialised we can submit jobs)
	*/
	renderer->initialise();

	/*
		Default assets are null assets (blank textures to fall back on when desired not found)
		Also load a font for the console to use for reporting initialisation progress
	*/
	assets.loadDefaultAssets();
	assets.loadAssets("/res/consolefontresource.xml");

	/*
		The console will start reporting progress of initialisation (loading assets)
	*/
	console = new Console();
	console->create();
	renderer->overlayRenderer.addLayer(console->getLayer()); // The console is a UI overlay

	/*
		This graphics job will 'recurse' (see Console::renderAtStartup) until initialisation is complete
	*/
	auto renderConsoleFunc = []() -> void {
		Engine::renderer->overlayRenderer.updateOverlayCommands();
		renderer->updateScreenDescriptorSets();
		renderer->updateScreenCommands();
		Engine::console->renderAtStartup();
	};

	/*
		Submit graphics job (console rendering)
	*/
	auto renderConsoleJob = new Job<>(renderConsoleFunc, defaultAsyncJobDoneFunc);
	Engine::threading->addGraphicsJob(renderConsoleJob, 1);

	/*
		Create bullet physics dynamic world
	*/
	physicsWorld.create();

	/*
		Initialise scripting environment and evaluate some functions in 'script.chai'
		For example camera movement is done within the script right now
	*/
	scriptEnv.initChai();
	scriptEnv.evalFile("./res/scripts/script.chai");

	PROFILE_END("init");

	PROFILE_START("assets");

	/*
		Load the rest of our assets defined in 'resources.xml'
	*/
	assets.loadAssets("/res/resources.xml");
	

	/*
		Wait for all assets to load and be transferred to GPU
		We wait here because in the next lines descriptor set updates require textures to be on the GPU
	*/
	while (!threading->allTransferJobsDone() || !threading->allRegularJobsDone())
	{
		// Submit any gpu transfer jobs that were children of DISK load jobs
		processAllMainThreadJobs();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	/// TODO: more flexible descriptor set updates that dont require all textures to be resident
	/*
		Update descriptor sets for all pipelines
	*/
	renderer->updateGBufferDescriptorSets();
	renderer->updateShadowDescriptorSets();
	renderer->updatePBRDescriptorSets();
	renderer->updatePBRCommands();

	PROFILE_END("assets");

	PROFILE_START("world");

	// Adding models to the world
	{
		std::string materialList[6] = { "bamboo", "greasymetal", "marble", "dirt", "mahog", "copper" };

		for (int i = 0; i < 3; ++i)
		{
			world.addModelInstance("box", "hollowbox" + std::to_string(i));
			int s = 100;
			int h = 100;
			int sh = s / 2;
			glm::fvec3 pos = glm::fvec3(s64(rand() % s) - sh, s64(rand() % h) / 5.f + 50.f, s64(rand() % s) - sh);
			//world.modelNames["hollowbox" + std::to_string(i)]->transform = glm::translate(glm::fmat4(1), glm::fvec3(-((i % 5) * 2), std::floor(int(i / (int)5) * 2), 0));

			Transform t;
			t.setTranslation(pos);
			t.setScale(glm::fvec3(10));
			t.updateMatrix();

			world.modelNames["hollowbox" + std::to_string(i)]->setTransform(t);
			world.modelNames["hollowbox" + std::to_string(i)]->setMaterial(assets.getMaterial(materialList[i % 6]));
			world.modelNames["hollowbox" + std::to_string(i)]->makePhysicsObject();
		}

		world.addModelInstance("pbrsphere", "pbrsphere");
		Transform t;
		t.setTranslation(glm::fvec3(4,0,0));
		t.setScale(glm::fvec3(10));
		t.updateMatrix();

		world.modelNames["pbrsphere"]->setTransform(t);
		world.modelNames["pbrsphere"]->setMaterial(assets.getMaterial("marble"));
		world.modelNames["pbrsphere"]->makePhysicsObject();

		world.addModelInstance("ground", "ground");
		t.setTranslation(glm::fvec3(0, 0, 0));
		t.updateMatrix();

		world.modelNames["ground"]->setTransform(t);
		world.modelNames["ground"]->setMaterial(assets.getMaterial("marble"));
		world.modelNames["ground"]->makePhysicsObject();

		//world.addModelInstance("monkey");
		//world.modelNames["monkey"]->transform = glm::translate(glm::fmat4(1), glm::fvec3(0, 10, 0));
	}

	initialised = 1;

	PROFILE_END("world");

	console->postMessage("Initialisation time : " + std::to_string(PROFILE_TO_S(PROFILE_GET_AVERAGE("init"))) + " seconds", glm::fvec3(0.8, 0.8, 0.3));
	console->postMessage("Asset load time     : " + std::to_string(PROFILE_TO_S(PROFILE_GET_AVERAGE("assets"))) + " seconds", glm::fvec3(0.8, 0.8, 0.3));
	console->postMessage("World loading time  : " + std::to_string(PROFILE_TO_S(PROFILE_GET_AVERAGE("world"))) + " seconds", glm::fvec3(0.8, 0.8, 0.3));

	/*
		Creating UI layer for displaying profiling stats
	*/
	uiLayer = new OLayer;
	uiLayer->create(glm::ivec2(1280, 720));
	renderer->overlayRenderer.addLayer(uiLayer);

	// For GPU and CPU profiling times
	Text* statsText = new Text;
	statsText->setName("stats");
	statsText->setFont(Engine::assets.getFont("consola"));
	statsText->setColour(glm::fvec4(0.2, 0.95, 0.2, 1));
	statsText->setCharSize(20);
	statsText->setString("");
	statsText->setPosition(glm::fvec2(0, 270));

	uiLayer->addElement(statsText);

	// For mutex lock wait times
	auto mutexStatsText = new Text;
	mutexStatsText->setName("mutexstats");
	mutexStatsText->setFont(Engine::assets.getFont("consola"));
	mutexStatsText->setColour(glm::fvec4(0.2, 0.95, 0.2, 1));
	mutexStatsText->setCharSize(20);
	mutexStatsText->setString("");
	mutexStatsText->setPosition(glm::fvec2(600, 450));

	uiLayer->addElement(mutexStatsText);

	// For per thread stats
	auto threadStatsText = new Text;
	threadStatsText->setName("threadstats");
	threadStatsText->setFont(Engine::assets.getFont("consola"));
	threadStatsText->setColour(glm::fvec4(0.2, 0.95, 0.2, 1));
	threadStatsText->setCharSize(20);
	threadStatsText->setString("");
	threadStatsText->setPosition(glm::fvec2(600, 590));

	uiLayer->addElement(threadStatsText);

	while (!threading->allAsyncJobsDone() || !threading->allGraphicsJobsDone() || !threading->allRegularJobsDone() || !threading->allTransferJobsDone())
	{
		processAllMainThreadJobs();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	/*
		These jobs are found in "CommonJobs.hpp"
		They will be re-added at the end of each frame
	*/
	//threading->addGraphicsJob(new Job<>(physicsToGPUJobFunc, defaultAsyncJobDoneFunc), 1);
	threading->addGraphicsJob(new Job<>(renderJobFunc, defaultAsyncJobDoneFunc), 1);
	threading->addJob(new Job<>(physicsJobFunc, defaultAsyncJobDoneFunc), 1);
	//threading->addJob(new Job<>(physicsToEngineJobFunc, defaultAsyncJobDoneFunc), 1);
	threading->addJob(new Job<>(scriptsJobFunc, defaultAsyncJobDoneFunc), 1);
	threading->addJob(new Job<>(cleanupJobsJobFunc, defaultAsyncJobDoneFunc), 1);

	threading->threadJobsProcessed[std::this_thread::get_id()] = 0;

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

		//PROFILE_START("msgevent");

		// Process OS messages (user input, etc) and translate them to engine events
		PROFILE_START("msgevent");
		window->processMessages();
		PROFILE_END("msgevent");

#ifdef _WIN32
		// We get keyboard state here for SHIFT+KEY events to work properly
		GetKeyboardState(Keyboard::keyState);
#endif

		eventLoop();

		console->update();

		// Display performance stats
		++frames;
		timeSinceLastStatsUpdate += frameTime.getSeconds();
		if (timeSinceLastStatsUpdate > 0.05f)
		{
			updatePerformanceStatsDisplay();
			timeSinceLastStatsUpdate = 0.f;
			frames = 0;
		}

		//PROFILE_END("msgevent");

		PROFILE_END("thread_" + Threading::getThisThreadIDString());
	}

	for (auto t : threading->workers)
	{
		t->join();
	}
	threading->cleanupJobs(); // Cleanup memory of finished jobs
	quit();
}

void Engine::eventLoop()
{
	// Process engine events
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
			if (ev.eventUnion.mouseEvent.code & Mouse::M_LEFT)
			{
				PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock()); /// TODO: this avoids a crash, but we dont want to hang the main thread for adding a constraint !!! maybe this should be a job, or better, batch all physics changes in one job
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
}

void Engine::processAllMainThreadJobs()
{
	JobBase* job;
	s64 timeUntilNextJob;
	while (threading->getGPUTransferJob(job, timeUntilNextJob))
	{
		job->run();
		++threading->threadJobsProcessed[std::this_thread::get_id()];
	}
	while (threading->getJob(job, timeUntilNextJob))
	{
		job->run();
		++threading->threadJobsProcessed[std::this_thread::get_id()];
	}
}

bool Engine::processNextMainThreadJob()
{
	JobBase* job;
	s64 timeUntilNextJob;
	if (threading->getGPUTransferJob(job, timeUntilNextJob))
	{
		job->run();
		++threading->threadJobsProcessed[std::this_thread::get_id()];
		return true;
	}
	else if (threading->getJob(job, timeUntilNextJob))
	{
		job->run();
		++threading->threadJobsProcessed[std::this_thread::get_id()];
		return true;
	}
	return false;
}

void Engine::updatePerformanceStatsDisplay()
{
	auto gBufferTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("gbuffer"));
	auto shadowTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("shadow"));
	auto pbrTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("pbr"));
	auto overlayTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("overlay"));
	auto overlayCombineTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("overlaycombine"));
	auto screenTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("screen"));

	auto gBufferTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("gbuffer"));
	auto shadowTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("shadow"));
	auto pbrTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("pbr"));
	auto overlayTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("overlay"));
	auto overlayCombineTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("overlaycombine"));
	auto screenTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("screen"));

	auto gBufferTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("gbuffer"));
	auto shadowTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("shadow"));
	auto pbrTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("pbr"));
	auto overlayTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("overlay"));
	auto overlayCombineTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("overlaycombine"));
	auto screenTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("screen"));

	auto totalGPUTime = gBufferTime + shadowTime + pbrTime + overlayTime + overlayCombineTime + screenTime;
	auto totalGPUMaxTime = gBufferTimeMax + shadowTimeMax + pbrTimeMax + overlayTimeMax + overlayCombineTimeMax + screenTimeMax;
	auto totalGPUMinTime = gBufferTimeMin + shadowTimeMin + pbrTimeMin + overlayTimeMin + overlayCombineTimeMin + screenTimeMin;

	auto gpuFPS = 1000.0 / totalGPUTime;
	auto gpuFPSMin = 1000.0 / totalGPUMinTime;
	auto gpuFPSMax = 1000.0 / totalGPUMaxTime;

	auto msgTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("msgevent"));
	auto cullTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("cullingdrawbuffer"));
	auto cmdsTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("commands"));
	//auto cmdsTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("setuprender"));
	auto submitTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("submitrender"));
	auto qWaitTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("qwaitidle"));
	auto physicsTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("physics"));
	auto scriptsTime = PROFILE_TO_MS(PROFILE_GET_AVERAGE("scripts"));

	auto msgTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("msgevent"));
	auto cullTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("cullingdrawbuffer"));
	auto cmdsTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("commands"));
	//auto cmdsTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("setuprender"));
	auto submitTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("submitrender"));
	auto qWaitTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("qwaitidle"));
	auto physicsTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("physics"));
	auto scriptsTimeMax = PROFILE_TO_MS(PROFILE_GET_MAX("scripts"));

	auto msgTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("msgevent"));
	auto cullTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("cullingdrawbuffer"));
	auto cmdsTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("commands"));
	//auto cmdsTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("setuprender"));
	auto submitTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("submitrender"));
	auto qWaitTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("qwaitidle"));
	auto physicsTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("physics"));
	auto scriptsTimeMin = PROFILE_TO_MS(PROFILE_GET_MIN("scripts"));

	auto stats = (Text*)uiLayer->getElement("stats");

	/*stats->setString(
		"---------------------------------------------------\n"
		"GBuffer pass   : " + std::to_string(gBufferTime) + "ms ( " + std::to_string(gBufferTimeMin) + " \\ " + std::to_string(gBufferTimeMax) + " )\n" +
		"Shadow pass    : " + std::to_string(shadowTime) + "ms ( " + std::to_string(shadowTimeMin) + " \\ " + std::to_string(shadowTimeMax) + " )\n" +
		"PBR pass       : " + std::to_string(pbrTime) + "ms ( " + std::to_string(pbrTimeMin) + " \\ " + std::to_string(pbrTimeMax) + " )\n" +
		"Overlay pass   : " + std::to_string(overlayTime) + "ms ( " + std::to_string(overlayTimeMin) + " \\ " + std::to_string(overlayTimeMax) + " )\n" +
		"OCombine pass  : " + std::to_string(overlayCombineTime) + "ms ( " + std::to_string(overlayCombineTimeMin) + " \\ " + std::to_string(overlayCombineTimeMax) + " )\n" +
		"Screen pass    : " + std::to_string(screenTime) + "ms ( " + std::to_string(screenTimeMin) + " \\ " + std::to_string(screenTimeMax) + " )\n" +

		"Total GPU time : " + std::to_string(totalGPUTime) + "ms ( " + std::to_string(totalGPUMinTime) + " \\ " + std::to_string(totalGPUMaxTime) + " )\n" +
		"GPU FPS        : " + std::to_string((int)gpuFPS) + " ( " + std::to_string((int)gpuFPSMax) + " \\ " + std::to_string((int)gpuFPSMin) + " )\n\n" +

		"---------------------------------------------------\n" +
		"User input     : " + std::to_string(msgTime) + "ms ( " + std::to_string(msgTimeMin) + " \\ " + std::to_string(msgTimeMax) + " )\n" +
		"Culling & draw : " + std::to_string(cullTime) + "ms ( " + std::to_string(cullTimeMin) + " \\ " + std::to_string(cullTimeMax) + " )\n" +
		"Commands       : " + std::to_string(cmdsTime) + "ms ( " + std::to_string(cmdsTimeMin) + " \\ " + std::to_string(cmdsTimeMax) + " )\n" +
		"Queue submit   : " + std::to_string(submitTime) + "ms ( " + std::to_string(submitTimeMin) + " \\ " + std::to_string(submitTimeMax) + " )\n" +
		"Queue idle     : " + std::to_string(qWaitTime) + "ms ( " + std::to_string(qWaitTimeMin) + " \\ " + std::to_string(qWaitTimeMax) + " )\n" +
		"Physics        : " + std::to_string(physicsTime) + "ms ( " + std::to_string(physicsTimeMin) + " \\ " + std::to_string(physicsTimeMax) + " )\n" +
		"Scripts        : " + std::to_string(scriptsTime) + "ms ( " + std::to_string(scriptsTimeMin) + " \\ " + std::to_string(scriptsTimeMax) + " )\n\n" +

		"Avg frame time : " + std::to_string((timeSinceLastStatsUpdate * 1000) / double(frames)) + "ms\n" +
		"FPS            : " + std::to_string((int)(double(frames) / timeSinceLastStatsUpdate))
	);*/

	stats->setString(
		"---------------------------\n"
		"GBuffer pass   : " + std::to_string(gBufferTime) + "ms\n" +
		"Shadow pass    : " + std::to_string(shadowTime) + "ms\n" +
		"PBR pass       : " + std::to_string(pbrTime) + "ms\n" +
		"Overlay pass   : " + std::to_string(overlayTime) + "ms\n" +
		"OCombine pass  : " + std::to_string(overlayCombineTime) + "ms\n" +
		"Screen pass    : " + std::to_string(screenTime) + "ms\n" +

		"Total GPU time : " + std::to_string(totalGPUTime) + "ms\n" +
		"GPU FPS        : " + std::to_string((int)gpuFPS) + "\n\n" +

		"---------------------------\n" +
		"User input     : " + std::to_string(msgTime) + "ms\n" +
		"Culling & draw : " + std::to_string(cullTime) + "ms\n" +
		"Commands       : " + std::to_string(cmdsTime) + "ms\n" +
		"Queue submit   : " + std::to_string(submitTime) + "ms\n" +
		"Queue idle     : " + std::to_string(qWaitTime) + "ms\n" +
		"Physics        : " + std::to_string(physicsTime) + "ms\n" +
		"Scripts        : " + std::to_string(scriptsTime) + "ms\n"
	);

	auto mutexStats = (Text*)uiLayer->getElement("mutexstats");

	mutexStats->setString(
		"----MUTEXES----------------\n"
		"---------------------------\n"
		"Physics        : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("physmutex"))) + "ms\n" +
		"Phys->Engine   : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("phystoenginemutex"))) + "ms\n" +
		"Phys->GPU      : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("phystogpumutex"))) + "ms\n" +
		"Transform      : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("transformmutex"))) + "ms\n"
	);

	auto threadStats = (Text*)uiLayer->getElement("threadstats");

	std::string threadStatsString;

	/*threadStatsString = "----THREADS-------------------\n------------------------------\n"
		"Thread_1 (main)   : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("thread_" + Threading::getThreadIDString(std::this_thread::get_id())))) + "ms ( " + std::to_string(threading->threadJobsProcessed[std::this_thread::get_id()]) + " jobs done )\n";

	int i = 2;
	for (auto t : threading->workers)
	{
		if (t == threading->workers.back()) // last thread is render thread so add that label
		{
			threadStatsString += std::string("Thread_") + std::to_string(i) + " (render) : " +
				std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("thread_" + Threading::getThreadIDString(threading->threadIDAssociations[i-1])))) + "ms ( " + std::to_string(threading->threadJobsProcessed[threading->threadIDAssociations[i - 1]]) + " jobs done )\n";
		}
		else
		{
			threadStatsString += std::string("Thread_") + std::to_string(i) + "          : " +
				std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("thread_" + Threading::getThreadIDString(threading->threadIDAssociations[i-1])))) + "ms ( " + std::to_string(threading->threadJobsProcessed[threading->threadIDAssociations[i - 1]]) + " jobs done )\n";
		}
	}*/

	threadStatsString = "----THREADS-------------------\n------------------------------\n"
		"Thread_1 (main)   : " + std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("thread_" + Threading::getThreadIDString(std::this_thread::get_id())))) + "ms ( " + std::to_string(PROFILE_TO_MS(PROFILE_GET_MAX("thread_" + Threading::getThreadIDString(std::this_thread::get_id())))) + " )\n";

	PROFILE_RESET("thread_" + Threading::getThreadIDString(std::this_thread::get_id()));

	int i = 2;
	for (auto t : threading->workers)
	{
		if (t == threading->workers.back()) // last thread is render thread so add that label
		{
			threadStatsString += std::string("Thread_") + std::to_string(i) + " (render) : " +
				std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("thread_" + Threading::getThreadIDString(threading->threadIDAssociations[i-1])))) + "ms ( " + std::to_string(PROFILE_TO_MS(PROFILE_GET_MAX("thread_" + Threading::getThreadIDString(threading->threadIDAssociations[i - 1])))) + " )\n";
		}
		else
		{
			threadStatsString += std::string("Thread_") + std::to_string(i) + "          : " +
				std::to_string(PROFILE_TO_MS(PROFILE_GET_AVERAGE("thread_" + Threading::getThreadIDString(threading->threadIDAssociations[i-1])))) + "ms ( " + std::to_string(PROFILE_TO_MS(PROFILE_GET_MAX("thread_" + Threading::getThreadIDString(threading->threadIDAssociations[i - 1])))) + " )\n";
		}
		PROFILE_RESET("thread_" + Threading::getThreadIDString(threading->threadIDAssociations[i-1]));
		++i;
	}

	threadStats->setString(threadStatsString);

	// Reset profiling info
	/// TODO: running average/min/max for last X frames
	PROFILE_RESET("msgevent");
	PROFILE_RESET("setuprender");
	PROFILE_RESET("submitrender");
	PROFILE_RESET("qwaitidle");
	PROFILE_RESET("physics");
	PROFILE_RESET("scripts");
	PROFILE_RESET("cullingdrawbuffer");
	PROFILE_RESET("commands");

	PROFILE_RESET("physmutex");
	PROFILE_RESET("phystoenginemutex");
	PROFILE_RESET("phystogpumutex");
	PROFILE_RESET("transformmutex");
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
	wci.posX = 1820-1280;
	wci.posY = 980-720;
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

	instance.setApplicationName("App");
	instance.setEngineName("Engine");
	instance.addExtension(VK_KHR_SURFACE_EXTENSION_NAME);
	instance.addExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	instance.addExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#ifdef ENABLE_VULKAN_VALIDATION
	instance.setDebugCallbackFunction(debugCallbackFunc);
	instance.addDebugReportLevel(vdu::Instance::DebugReportLevel::Warning);
	instance.addDebugReportLevel(vdu::Instance::DebugReportLevel::Error);
	instance.addLayer("VK_LAYER_LUNARG_standard_validation");
#endif

	instance.create();

	vkInstance = instance.getInstanceHandle();
}

/*
	@brief	Queries vulkan instance for physical device information, picks best one
*/
void Engine::queryVulkanPhysicalDeviceDetails()
{
	vdu::enumeratePhysicalDevices(instance, allPhysicalDevices);
	physicalDevice = &allPhysicalDevices.front();
	physicalDevice->querySurfaceCapabilities(window->vkSurface);
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
	instance.destroy();
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
Time Engine::frameTime(0);
ScriptEnv Engine::scriptEnv;
Threading* Engine::threading;
std::atomic_char Engine::initialised = 0;
std::mutex Engine::waitForProfilerInitMutex;
VkResult Engine::lastVulkanResult;
double Engine::timeSinceLastStatsUpdate = 0.f;
int Engine::frames = 0;
vdu::Instance Engine::instance;
vdu::PhysicalDevice* Engine::physicalDevice;
std::vector<vdu::PhysicalDevice> Engine::allPhysicalDevices;
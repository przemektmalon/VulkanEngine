#pragma once
#include "PCH.hpp"
#include "Profiler.hpp"
#include "Engine.hpp"
#include "Threading.hpp"
#include "PhysicsWorld.hpp"
#include "Renderer.hpp"
#include "Console.hpp"

static VoidJobType physicsJobFunc;
static VoidJobType physicsToEngineJobFunc;
static VoidJobType physicsToGPUJobFunc;
static VoidJobType renderJobFunc;
static VoidJobType scriptsJobFunc;
static VoidJobType cleanupJobsJobFunc;

static void initialiseCommonJobs()
{
	physicsJobFunc = []() -> void {
		PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock());
		PROFILE_START("physics");
		static u64 endTime = Engine::clock.now();
		u64 timeSinceLastPhysics = Engine::clock.now() - endTime;
		Engine::physicsWorld.step(float(timeSinceLastPhysics) / 1000.f);
		endTime = Engine::clock.now();
		PROFILE_END("physics");
		Engine::threading->physBulletMutex.unlock();

		PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock());
		Engine::physicsWorld.updateModels();
		Engine::threading->physBulletMutex.unlock();

		PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
		//PROFILE_START("physics");
		Engine::renderer->updateTransformBuffer();
		//PROFILE_END("physics");
		Engine::threading->physToEngineMutex.unlock();

		if (Engine::engineRunning)
		{
			u64 microsecondsBetweenPhysicsUpdates = 1000000 / 120;
			//u64 microsecondsBetweenPhysicsUpdates = 0;

			auto nextPhysicsJob = new Job<>(physicsJobFunc);
			nextPhysicsJob->setScheduledTime(endTime + microsecondsBetweenPhysicsUpdates);
			Engine::threading->addCPUJob(nextPhysicsJob);
		}
	};

	physicsToEngineJobFunc = []() -> void {
		//PROFILE_START("physics");
		PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock());
		Engine::physicsWorld.updateModels();
		Engine::threading->physBulletMutex.unlock();
		//PROFILE_END("physics");

		if (Engine::engineRunning)
		{
			u64 microsecondsBetweenPhysicsUpdates = 1000000 / 120;
			//u64 microsecondsBetweenPhysicsUpdates = 0;

			auto nextPhysicsJob = new Job<>(physicsToEngineJobFunc);
			nextPhysicsJob->setScheduledTime(Engine::clock.now() + microsecondsBetweenPhysicsUpdates);
			Engine::threading->addCPUJob(nextPhysicsJob);
		}
	};

	physicsToGPUJobFunc = []() -> void {
		PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
		//PROFILE_START("physics");
		Engine::renderer->updateTransformBuffer();
		//PROFILE_END("physics");
		Engine::threading->physToEngineMutex.unlock();

		if (Engine::engineRunning)
		{
			u64 microsecondsBetweenPhysicsUpdates = 1000000 / 120;
			//u64 microsecondsBetweenPhysicsUpdates = 0;

			auto nextPhysicsJob = new Job<>(physicsToGPUJobFunc, JobBase::GPU);
			nextPhysicsJob->setScheduledTime(Engine::clock.now() + microsecondsBetweenPhysicsUpdates);
			Engine::threading->addGPUJob(nextPhysicsJob);
		}
	};

	renderJobFunc = []() -> void {
		PROFILE_START("qwaitidle");
		VK_CHECK_RESULT(vkQueueWaitIdle(Engine::renderer->graphicsQueue));
		PROFILE_END("qwaitidle");

		Engine::renderer->lightManager.sunLight.calcProjs();
		Engine::renderer->lightManager.updateSunLight();
		Engine::renderer->updateGBufferDescriptorSets();

		PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
		PROFILE_START("cullingdrawbuffer");
		Engine::world.frustumCulling(&Engine::camera);
		Engine::renderer->populateDrawCmdBuffer(); // Mutex with engine model transform update
		Engine::threading->physToEngineMutex.unlock();
		PROFILE_END("cullingdrawbuffer");

		PROFILE_START("setuprender");
		PROFILE_START("commands");
		Engine::renderer->updateConfigs();
		Engine::renderer->updateGBufferCommands();
		Engine::renderer->updateShadowCommands(); // Mutex with engine model transform update
		Engine::renderer->overlayRenderer.updateOverlayCommands(); // Mutex with any overlay additions/removals
		PROFILE_MUTEX("transformmutex", Engine::threading->instanceTransformMutex.lock());
		Engine::renderer->updateCameraBuffer();
		Engine::threading->instanceTransformMutex.unlock();
		PROFILE_END("commands");

		PROFILE_MUTEX("phystogpumutex", Engine::threading->physToGPUMutex.lock());
		Engine::renderer->render();
		Engine::threading->physToGPUMutex.unlock();

		PROFILE_END("setuprender");

		if (Engine::engineRunning)
			Engine::threading->addGPUJob(new Job<>(renderJobFunc, JobBase::GPU));
	};

	scriptsJobFunc = []() -> void {
		PROFILE_START("scripts");
		PROFILE_MUTEX("transformmutex", Engine::threading->instanceTransformMutex.lock());
		if (!Engine::console->isActive())
		{
			try {
				Engine::scriptEnv.evalString("updateCamera()");
			}
			catch (chaiscript::exception::eval_error e)
			{
				std::cout << e.what() << std::endl;
			}
		}
		Engine::threading->instanceTransformMutex.unlock();
		PROFILE_END("scripts");

		if (Engine::engineRunning)
		{
			auto nextScriptsJob = new Job<>(scriptsJobFunc);
			nextScriptsJob->setScheduledTime(Engine::clock.now() + 10000);
			Engine::threading->addCPUJob(nextScriptsJob);
		}
	};

	cleanupJobsJobFunc = []() -> void {

		Engine::threading->cleanupJobs();

		if (Engine::engineRunning)
		{
			auto nextCleanupJob = new Job<>(cleanupJobsJobFunc);
			nextCleanupJob->setScheduledTime(Engine::clock.now() + 10000);
			Engine::threading->addCPUJob(nextCleanupJob);
		}
	};

}

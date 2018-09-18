#pragma once
#include "PCH.hpp"
#include "Profiler.hpp"
#include "Engine.hpp"
#include "Threading.hpp"
#include "PhysicsWorld.hpp"
#include "Renderer.hpp"
#include "Console.hpp"

static VoidJobType physicsJobFunc;
static VoidJobType physicsToGPUJobFunc;
static VoidJobType renderJobFunc;
static VoidJobType scriptsJobFunc;
static VoidJobType cleanupJobsJobFunc;

static void initialiseCommonJobs()
{
	physicsJobFunc = []() -> void {

		auto& physics = Engine::physicsWorld;
		auto& threading = Engine::threading;
		auto& renderer = Engine::renderer;
		auto& clock = Engine::clock;

		PROFILE_MUTEX("physmutex", threading->physBulletMutex.lock());
		PROFILE_START("physics");

		physics.updateAddedObjects(); // Objects just added to the physics world

		static u64 endTime = clock.now(); // Static var for timing physics

		u64 timeSinceLastPhysics = clock.now() - endTime;
		physics.step(float(timeSinceLastPhysics) / 1000.f); // Step physics by appropriate time
		endTime = clock.now();

		physics.updateModels(); // Get transform data from bullet to engine

		threading->physBulletMutex.unlock();

		PROFILE_MUTEX("phystoenginemutex", threading->physToEngineMutex.lock());
		renderer->updateTransformBuffer();
		threading->physToEngineMutex.unlock();

		PROFILE_END("physics");

		if (Engine::engineRunning)
		{
			u64 microsecondsBetweenPhysicsUpdates = 1000000 / 120;

			auto nextPhysicsJob = new Job<>(physicsJobFunc);
			nextPhysicsJob->setScheduledTime(endTime + microsecondsBetweenPhysicsUpdates);
			threading->addCPUJob(nextPhysicsJob);
		}
	};

	physicsToGPUJobFunc = []() -> void {

		auto& physics = Engine::physicsWorld;
		auto& threading = Engine::threading;
		auto& renderer = Engine::renderer;
		auto& clock = Engine::clock;

		PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
		PROFILE_START("physics");
		renderer->updateTransformBuffer();
		PROFILE_END("physics");
		threading->physToEngineMutex.unlock();

		if (Engine::engineRunning)
		{
			u64 microsecondsBetweenPhysicsUpdates = 1000000 / 120;
			//u64 microsecondsBetweenPhysicsUpdates = 0;

			auto nextPhysicsJob = new Job<>(physicsToGPUJobFunc, JobBase::GPUTransfer);
			nextPhysicsJob->setScheduledTime(clock.now() + microsecondsBetweenPhysicsUpdates);
			threading->addGPUTransferJob(nextPhysicsJob);
		}
	};

	renderJobFunc = []() -> void {

		auto& renderer = Engine::renderer;
		auto& assets = Engine::assets;
		auto& threading = Engine::threading;
		auto& world = Engine::world;

		PROFILE_START("qwaitidle");
		renderer->lGraphicsQueue.waitIdle();
		PROFILE_END("qwaitidle");

		renderer->lightManager.sunLight.calcProjs();
		renderer->lightManager.updateSunLight();

		threading->addMaterialMutex.lock();

		renderer->overlayRenderer.cleanupLayerElements();
		renderer->executeFenceDelayedActions();

		renderer->updateMaterialDescriptors();

		threading->addMaterialMutex.unlock();

		PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
		PROFILE_START("cullingdrawbuffer");
		world.frustumCulling(&Engine::camera);
		renderer->populateDrawCmdBuffer(); // Mutex with engine model transform update
		threading->physToEngineMutex.unlock();
		PROFILE_END("cullingdrawbuffer");

		PROFILE_START("setuprender");
		PROFILE_START("commands");
		renderer->updateConfigs();
		renderer->updateGBufferCommands();
		renderer->updateShadowCommands(); // Mutex with engine model transform update
		renderer->overlayRenderer.updateOverlayCommands(); // Mutex with any overlay additions/removals
		PROFILE_MUTEX("transformmutex", Engine::threading->instanceTransformMutex.lock());
		renderer->updateCameraBuffer();
		threading->instanceTransformMutex.unlock();
		PROFILE_END("commands");

		PROFILE_MUTEX("phystogpumutex", Engine::threading->physToGPUMutex.lock());
		renderer->render();
		threading->physToGPUMutex.unlock();

		PROFILE_END("setuprender");

		if (Engine::engineRunning)
			threading->addGPUJob(new Job<>(renderJobFunc, JobBase::GPU));
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

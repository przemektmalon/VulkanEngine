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
		Engine::physicsWorld.updateAddedObjects();
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

			auto nextPhysicsJob = new Job<>(physicsToGPUJobFunc, JobBase::GPUTransfer);
			nextPhysicsJob->setScheduledTime(Engine::clock.now() + microsecondsBetweenPhysicsUpdates);
			Engine::threading->addGPUTransferJob(nextPhysicsJob);
		}
	};

	renderJobFunc = []() -> void {
		PROFILE_START("qwaitidle");
		VK_CHECK_RESULT(vkQueueWaitIdle(Engine::renderer->graphicsQueue));
		PROFILE_END("qwaitidle");

		Engine::renderer->lightManager.sunLight.calcProjs();
		Engine::renderer->lightManager.updateSunLight();

		Engine::threading->addMaterialMutex.lock();

		Engine::renderer->overlayRenderer.cleanupLayerElements();
		Engine::renderer->executeFenceDelayedActions();
		Engine::renderer->destroyDelayedBuffers();

		u32 i = 0;
		for (auto& material : Engine::assets.materials)
		{
			if (!material.second.checkAvailability(Asset::AWAITING_DESCRIPTOR_UPDATE))
			{
				i += 2;
				continue;
			}

			material.second.getAvailability() &= ~Asset::AWAITING_DESCRIPTOR_UPDATE;

			auto updater = Engine::renderer->gBufferDescriptorSet.makeUpdater();
			auto texturesUpdate = updater->addImageUpdate("textures", i, 2);

			texturesUpdate[0].sampler = Engine::renderer->textureSampler;
			texturesUpdate[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			texturesUpdate[1].sampler = Engine::renderer->textureSampler;
			texturesUpdate[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			if (material.second.albedoSpec->getHandle())
				texturesUpdate[0].imageView = material.second.albedoSpec->getView();
			else
				texturesUpdate[0].imageView = Engine::assets.getTexture("blank")->getView();

			if (material.second.normalRough)
			{
				if (material.second.normalRough->getView())
					texturesUpdate[1].imageView = material.second.normalRough->getView();
				else
					texturesUpdate[1].imageView = Engine::assets.getTexture("black")->getView();
			}
			else
				texturesUpdate[1].imageView = Engine::assets.getTexture("black")->getView();

			i += 2;

			Engine::renderer->gBufferDescriptorSet.submitUpdater(updater);
			Engine::renderer->gBufferDescriptorSet.destroyUpdater(updater);
		}

		Engine::threading->addMaterialMutex.unlock();

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
		try {
			Engine::scriptEnv.evalString("gameTick()");
		}
		catch (chaiscript::exception::eval_error e)
		{
			std::cout << e.what() << std::endl;
		}
		Engine::threading->instanceTransformMutex.unlock();
		PROFILE_END("scripts");
		Engine::scriptTickTime.setMicroSeconds(PROFILE_GET_LAST("scripts"));
		//std::cout << PROFILE_GET_LAST("scripts") << std::endl;

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

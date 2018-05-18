#pragma once
#include "PCH.hpp"
#include "Profiler.hpp"
#include "Engine.hpp"
#include "Threading.hpp"
#include "PhysicsWorld.hpp"
#include "Renderer.hpp"

static auto physicsJobFunc = []() -> void {
	PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock());
	PROFILE_START("physics");
	Engine::physicsWorld.step(Engine::frameTime);
	PROFILE_END("physics");
	Engine::threading->physBulletMutex.unlock();

};

static auto physicsToEngineJobFunc = []() -> void {
	PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
	PROFILE_MUTEX("transformmutex", Engine::threading->instanceTransformMutex.lock());
	PROFILE_START("physics");
	Engine::physicsWorld.updateModels();
	PROFILE_END("physics");
	Engine::threading->instanceTransformMutex.unlock();
	Engine::threading->physToEngineMutex.unlock();
};

static auto physicsToGPUJobFunc = []() -> void {
	PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
	PROFILE_MUTEX("phystogpumutex", Engine::threading->physToGPUMutex.lock());
	PROFILE_START("physics");
	Engine::renderer->updateTransformBuffer();
	PROFILE_END("physics");
	Engine::threading->physToGPUMutex.unlock();
	Engine::threading->physToEngineMutex.unlock();
};

static auto renderJobFunc = []() -> void {
	PROFILE_MUTEX("phystogpumutex", Engine::threading->physToGPUMutex.lock());
	Engine::renderer->render();
	Engine::threading->physToGPUMutex.unlock();
};

static auto renderPrepareJobBFunc = []() -> void {
	PROFILE_START("setuprender");

	Engine::renderer->overlayRenderer.updateOverlayCommands(); // Mutex with any overlay additions/removals
	Engine::renderer->updateCameraBuffer();

	auto nextJob = new Job<>(renderJobFunc, defaultJobDoneFunc);
	Engine::threading->addGraphicsJob(nextJob);

	PROFILE_END("setuprender");
};

static auto renderPrepareJobAFunc = []() -> void {
	PROFILE_START("setuprender");

	PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
	Engine::world.frustumCulling(&Engine::camera);
	Engine::renderer->populateDrawCmdBuffer(); // Mutex with engine model transform update
	Engine::threading->physToEngineMutex.unlock();

	Engine::renderer->updateShadowCommands(); // Mutex with engine model transform update
	Engine::renderer->updateGBufferCommands();

	auto nextJob = new Job<>(renderPrepareJobBFunc, defaultJobDoneFunc);
	Engine::threading->addJob(nextJob);

	PROFILE_END("setuprender");
};

static auto scriptsJobFunc = []() -> void {
	PROFILE_START("scripts");
	PROFILE_MUTEX("transformmutex", Engine::threading->instanceTransformMutex.lock());
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
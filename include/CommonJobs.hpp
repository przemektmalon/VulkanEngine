#pragma once
#include "PCH.hpp"
#include "Profiler.hpp"
#include "Engine.hpp"
#include "Threading.hpp"
#include "PhysicsWorld.hpp"
#include "Renderer.hpp"
#include "Console.hpp"

static auto physicsJobFunc = []() -> void {
	PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock());
	PROFILE_START("physics");
	static u64 endTime = Engine::clock.now();
	u64 timeSinceLastPhysics = Engine::clock.now() - endTime;
	Engine::physicsWorld.step(float(timeSinceLastPhysics)/1000.f);
	endTime = Engine::clock.now();
	PROFILE_END("physics");
	Engine::threading->physBulletMutex.unlock();


};

static auto physicsToEngineJobFunc = []() -> void {
	//PROFILE_START("physics");
	PROFILE_MUTEX("physmutex", Engine::threading->physBulletMutex.lock());
	Engine::physicsWorld.updateModels();
	Engine::threading->physBulletMutex.unlock();
	//PROFILE_END("physics");
};

static auto physicsToGPUJobFunc = []() -> void {
	PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
	//PROFILE_START("physics");
	Engine::renderer->updateTransformBuffer();
	//PROFILE_END("physics");
	Engine::threading->physToEngineMutex.unlock();
};

static auto renderJobFunc = []() -> void {

	PROFILE_START("qwaitidle");
	vkQueueWaitIdle(Engine::renderer->graphicsQueue);
	PROFILE_END("qwaitidle");

	PROFILE_START("commands");
	PROFILE_MUTEX("phystoenginemutex", Engine::threading->physToEngineMutex.lock());
	PROFILE_START("cullingdrawbuffer");
	Engine::world.frustumCulling(&Engine::camera);
	Engine::renderer->populateDrawCmdBuffer(); // Mutex with engine model transform update
	Engine::threading->physToEngineMutex.unlock();
	PROFILE_END("cullingdrawbuffer");

	PROFILE_START("setuprender");
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
};

static auto scriptsJobFunc = []() -> void {
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
};
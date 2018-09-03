#include "PCH.hpp"
#include "EngineConfig.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "Threading.hpp"

void postMessage(std::string msg, glm::fvec3 col)
{
	Engine::console->postMessage(msg, col);
}

void EngineConfig::Render::setResolution(glm::ivec2 set)
{
	if (set.x <= 0 || set.y <= 0) {
		postMessage("Invalid Render resolution setting. Must be greater than 0", ERROR_COL);
		return;
	}
	resolution = set;
	auto resizeJobFunc = std::bind([](glm::ivec2 res) -> void {
		Engine::window->resize(res.x, res.y);
		Engine::physicalDevice->querySurfaceCapabilities(Engine::window->vkSurface);
		Engine::config.changedSpecials.insert(Render_Resolution);
	},set);
	Engine::threading->addGPUJob(new Job<decltype(resizeJobFunc)>(resizeJobFunc, JobBase::GPU));
}

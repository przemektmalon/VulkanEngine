#include "PCH.hpp"
#include "EngineConfig.hpp"
#include "Engine.hpp"

void postMessage(std::string msg, glm::fvec3 col)
{
	Engine::console->postMessage(msg, col);
}

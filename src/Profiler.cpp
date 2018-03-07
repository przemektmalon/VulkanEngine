#include "Profiler.hpp"
#include "Engine.hpp"

void Profiler::start(std::string id)
{
	times[id].first.setMicroSeconds(Engine::clock.now());
}

void Profiler::end(std::string id)
{
	times[id].second.setMicroSeconds(Engine::clock.now() - times[id].first.getMicroSeconds());
}

std::unordered_map<std::string, std::pair<Time, Time>> Profiler::times;
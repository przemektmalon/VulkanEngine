#pragma once
#include "PCH.hpp"
#include "Time.hpp"

#define PROFILE_THIS(func,name) Profiler::start(name); func; Engine::profiler.end(name);
#define PROFILE_START(name) Profiler::start(name);
#define PROFILE_END(name) Profiler::end(name)

#define PROFILE_TIME(name) Profiler::getTime(name).getSeconds()

class Profiler
{
public:
	Profiler() {}
	~Profiler() {}

	static void start(std::string id);
	static void end(std::string id);

	static Time getTime(std::string id)
	{
		return times[id].second;
	}

private:

	static std::unordered_map<std::string, std::pair<Time,Time>> times;
};
#include "PCH.hpp"
#include "Profiler.hpp"
#include "Engine.hpp"

void Profiler::prealloc(std::vector<std::thread::id>& threadIDs, std::vector<std::string>& tags)
{
	for (auto& tag : tags)
	{
		profiles.try_emplace(tag);
	}

	for (auto& threadID : threadIDs)
	{
		for (auto& tag : tags)
		{
			startTimes[threadID][tag] = 0;
		}
	}
}

void Profiler::start(std::string id)
{
	startTimes[std::this_thread::get_id()][id] = Engine::clock.now();
}

void Profiler::end(std::string id)
{
	u32 time = Engine::clock.now() - startTimes[std::this_thread::get_id()][id];

	profiles[id].totalMicroseconds += time;
	profiles[id].numSamples += 1;

	profiles[id].minMaxMutex.lock();
	if (time > profiles[id].maximumMicroseconds.load())
		profiles[id].maximumMicroseconds.store(time);
	if (time < profiles[id].minimumMicroseconds.load())
		profiles[id].minimumMicroseconds.store(time);
	profiles[id].minMaxMutex.unlock();
}

void Profiler::addGPUTime(std::string id, u64 start, u64 end)
{
	/// TODO: query GPU for timestamp resolution and change '1000' accordingly
	u32 time = (end - start) / 1000; // We store microseconds, (my) GPU gives nanosecond timestamps
	
	profiles[id].totalMicroseconds += time;
	profiles[id].numSamples += 1;

	profiles[id].minMaxMutex.lock();
	if (time > profiles[id].maximumMicroseconds.load())
		profiles[id].maximumMicroseconds.store(time);
	if (time < profiles[id].minimumMicroseconds.load())
		profiles[id].minimumMicroseconds.store(time);
	profiles[id].minMaxMutex.unlock();
}

std::unordered_map<std::thread::id, std::unordered_map<std::string, u64>> Profiler::startTimes;
std::unordered_map<std::string, Profiler::Profile> Profiler::profiles;
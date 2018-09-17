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

	auto& profile = profiles[id];

	char32_t pastEndIndex = RUNNING_AVERAGE_COUNT;
	profile.circBufferHead++;
	profile.circBufferHead.compare_exchange_strong(pastEndIndex, 0);
	profile.circBuffer[profile.circBufferHead] = time;

	profile.totalMicroseconds += time;
	profile.numSamples += 1;

	profile.minMaxMutex.lock();
	if (time > profile.maximumMicroseconds.load())
		profile.maximumMicroseconds.store(time);
	if (time < profile.minimumMicroseconds.load())
		profile.minimumMicroseconds.store(time);
	profile.minMaxMutex.unlock();
}

void Profiler::addGPUTime(std::string id, u64 start, u64 end)
{
	/// TODO: query GPU for timestamp resolution and change '1000' accordingly
	u32 time = (end - start) / 1000; // We store microseconds, (my) GPU gives nanosecond timestamps
	
	auto& profile = profiles[id];

	char32_t pastEndIndex = RUNNING_AVERAGE_COUNT;
	profile.circBufferHead++;
	profile.circBufferHead.compare_exchange_strong(pastEndIndex, 0);
	profile.circBuffer[profile.circBufferHead] = time;

	profile.totalMicroseconds += time;
	profile.numSamples += 1;

	profile.minMaxMutex.lock();
	if (time > profile.maximumMicroseconds.load())
		profile.maximumMicroseconds.store(time);
	if (time < profile.minimumMicroseconds.load())
		profile.minimumMicroseconds.store(time);
	profile.minMaxMutex.unlock();
}

std::unordered_map<std::thread::id, std::unordered_map<std::string, u64>> Profiler::startTimes;
std::unordered_map<std::string, Profiler::Profile> Profiler::profiles;
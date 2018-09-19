#pragma once
#include "PCH.hpp"
#include "Time.hpp"

#define PROFILE_THIS(func,name) Profiler::start(name); func; Profiler::end(name);
#define PROFILE_START(name) Profiler::start(name);
#define PROFILE_END(name) Profiler::end(name);

#define PROFILE_GET_RUNNING_TOTAL(name) Profiler::getProfile(name).getRunningTotal()
#define PROFILE_GET_RUNNING_AVERAGE(name) Profiler::getProfile(name).getRunningAverage()
#define PROFILE_GET_RUNNING_MIN(name) Profiler::getProfile(name).getRunningMinimum()
#define PROFILE_GET_RUNNING_MAX(name) Profiler::getProfile(name).getRunningMaximum()

#define PROFILE_GET_LAST(name) Profiler::getProfile(name).getLastTime()

#define PROFILE_TO_MS(us) (us * 0.001)
#define PROFILE_TO_S(us) (us * 0.000001)

#define PROFILE_GPU_ADD_TIME(name, start, end) Profiler::addGPUTime(name, start, end);

#define PROFILE_MUTEX(name, mutex) Profiler::start(name); mutex; Profiler::end(name);

/// TODO:   In the future we might want to profile specific parts of scripts that are loaded dynamically at run-time
///			We therefore cannot preallocate the tags of these profiles beforehand
///			We need to have a thread safe tag addition for this scenario
///			Along with this the profiler will need to be exposed to chaiscript
///			Also the script shouldnt be responsible for resetting the profiles

#define RUNNING_AVERAGE_COUNT 120

class Profiler
{
private:
	struct Profile
	{
		Profile() { memset(circBuffer.data(), 0, RUNNING_AVERAGE_COUNT * sizeof(u32)); }

		std::atomic_char32_t totalMicroseconds = 0;
		std::atomic_char32_t minimumMicroseconds = 0;
		std::atomic_char32_t maximumMicroseconds = 0;
		std::atomic_char32_t circBufferHead = 0;
		std::atomic_char32_t numSamples;

		std::array<u32, RUNNING_AVERAGE_COUNT> circBuffer;

		// Since we need order (> & <) operations we need a lock as they are no atomic exchanges with these
		std::mutex minMaxMutex;

		double getRunningAverage()
		{
			u32 total = 0;
			for (auto time : circBuffer)
				total += time;
			return static_cast<double>(total) / static_cast<double>(RUNNING_AVERAGE_COUNT);
		}
		double getRunningMaximum()
		{
			u32 max = 0;
			for (auto time : circBuffer)
				if (time > max)
					max = time;
			return static_cast<double>(max);
		}
		double getRunningMinimum()
		{
			u32 min = std::numeric_limits<u32>::max();
			for (auto time : circBuffer)
				if (time < min)
					min = time;
			return static_cast<double>(min);
		}
		double getRunningTotal()
		{
			u32 total = 0;
			for (auto time : circBuffer)
				total += time;
			return total;
		}
		double getLastTime()
		{
			return circBuffer[circBufferHead];
		}
	};

public:
	Profiler() {}
	~Profiler() {}

	static void prealloc(std::vector<std::thread::id>& threadIDs, std::vector<std::string>& tags);
	static void start(std::string id);
	static void end(std::string id);

	static void addGPUTime(std::string id, u64 start, u64 end);

	static Profile& const getProfile(std::string id)
	{
		return profiles[id];
	}

private:

	static std::unordered_map<std::thread::id, std::unordered_map<std::string, u64>> startTimes; // Start times for each thread for each tag
	static std::unordered_map<std::string, Profile> profiles; // Profiles for each tag
};
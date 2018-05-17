#pragma once
#include "PCH.hpp"
#include "Time.hpp"

#define PROFILE_THIS(func,name) Profiler::start(name); func; Profiler::end(name);
#define PROFILE_START(name) Profiler::start(name);
#define PROFILE_END(name) Profiler::end(name);

#define PROFILE_RESET(name) Profiler::getProfile(name).reset();

#define PROFILE_GET(name) Profiler::getProfile(name)
#define PROFILE_GET_TOTAL(name) Profiler::getProfile(name).getTotal()
#define PROFILE_GET_AVERAGE(name) Profiler::getProfile(name).getAverage()
#define PROFILE_GET_MIN(name) Profiler::getProfile(name).getMinimum()
#define PROFILE_GET_MAX(name) Profiler::getProfile(name).getMaximum()

#define PROFILE_TO_MS(us) (us * 0.001)
#define PROFILE_TO_S(us) (us * 0.000001)

#define PROFILE_GPU_ADD_TIME(name, start, end) Profiler::addGPUTime(name, start, end);

/// TODO:   In the future we might want to profile specific parts of scripts that are loaded dynamically at run-time
///			We therefore cannot preallocate the tags of these profiles beforehand
///			We need to have a thread safe tag addition for this scenario
///			Along with this the profiler will need to be exposed to chaiscript
///			Also the script shouldnt be responsible for resetting the profiles

/// TODO:	Think about a running average instead of average of intervals

class Profiler
{
private:
	struct Profile
	{
		Profile() : totalMicroseconds(0), minimumMicroseconds(0), maximumMicroseconds(0), numSamples(0) {}

		std::atomic_char32_t totalMicroseconds;
		std::atomic_char32_t minimumMicroseconds;
		std::atomic_char32_t maximumMicroseconds;
		std::atomic_char32_t numSamples;

		// Since we need order (> & <) operations we need a lock as they are no atomic exchanges with these
		std::mutex minMaxMutex;

		void reset()
		{
			totalMicroseconds = 0;
			minimumMicroseconds = std::numeric_limits<u32>::max();
			maximumMicroseconds = 0;
			numSamples = 0;
		}

		double getTotal() { return static_cast<double>(totalMicroseconds.load()); }
		double getAverage() { return static_cast<double>(totalMicroseconds.load()) / static_cast<double>(numSamples.load()); }
		double getMinimum() { return static_cast<double>(minimumMicroseconds.load()); }
		double getMaximum() { return static_cast<double>(maximumMicroseconds.load()); }
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
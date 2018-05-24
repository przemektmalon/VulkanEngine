#include "PCH.hpp"
#include "Threading.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Profiler.hpp"

Threading::Threading(int pNumThreads) : workers(pNumThreads-1)
{
	totalJobsAdded.store(0);
	totalJobsFinished.store(0);
	totalTransferJobsAdded.store(0);
	totalTransferJobsFinished.store(0);
	totalGraphicsJobsAdded.store(0);
	totalGraphicsJobsFinished.store(0);
	totalAsyncJobsAdded.store(0);
	totalAsyncJobsFinished.store(0);
	for (int i = 0; i < pNumThreads - 2; ++i)
	{
		workers[i] = new std::thread(&Threading::update, this);
	}
	workers[pNumThreads - 2] = new std::thread(&Threading::updateGraphics, this);
}

Threading::~Threading()
{
	/// TODO: notify threads to force terminate ??
	for (auto w : workers)
	{
		w->join();
	}
}

void Threading::addJob(JobBase * jobToAdd, int async)
{
	jobsQueueMutex.lock();
	totalJobsAdded.fetch_add(1 - async);
	totalAsyncJobsAdded.fetch_add(async);
	jobs.push_back(jobToAdd);
	jobsQueueMutex.unlock();
}

void Threading::addGraphicsJob(JobBase * jobToAdd, int async)
{
	graphicsJobsQueueMutex.lock();
	totalGraphicsJobsAdded.fetch_add(1 - async);
	totalAsyncJobsAdded.fetch_add(async);
	graphicsJobs.push_back(jobToAdd);
	graphicsJobsQueueMutex.unlock();
}

void Threading::addGPUTransferJob(JobBase * jobToAdd, int async)
{
	gpuTransferJobsQueueMutex.lock();
	totalTransferJobsAdded.fetch_add(1 - async);
	totalAsyncJobsAdded.fetch_add(async);
	gpuTransferJobs.push_back(jobToAdd);
	gpuTransferJobsQueueMutex.unlock();
}

bool Threading::getJob(JobBase *& job, s64& timeUntilNextJob)
{
	jobsQueueMutex.lock();
	if (jobs.empty())
	{
		jobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = jobs.begin();
	timeUntilNextJob = std::max((s64)0, (*jobItr)->getScheduledTime() - (s64)Engine::clock.now());
	while (timeUntilNextJob != 0)
	{
		++jobItr;
		if (jobItr == jobs.end())
		{
			jobsQueueMutex.unlock();
			return false;
		}
		timeUntilNextJob = std::max((s64)0, std::min((*jobItr)->getScheduledTime() - (s64)Engine::clock.now(), timeUntilNextJob));
	}
	job = *jobItr;
	jobs.erase(jobItr);
	jobsQueueMutex.unlock();
	return true;
}

bool Threading::getGraphicsJob(JobBase *& job, s64& timeUntilNextJob)
{
	graphicsJobsQueueMutex.lock();
	if (graphicsJobs.empty())
	{
		graphicsJobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = graphicsJobs.begin();
	timeUntilNextJob = std::max((s64)0, (*jobItr)->getScheduledTime() - (s64)Engine::clock.now());
	while (timeUntilNextJob > 0)
	{
		++jobItr;
		if (jobItr == graphicsJobs.end())
		{
			graphicsJobsQueueMutex.unlock();
			return false;
		}
		timeUntilNextJob = std::max((s64)0, std::min((*jobItr)->getScheduledTime() - (s64)Engine::clock.now(), timeUntilNextJob));
	}
	job = *jobItr;
	graphicsJobs.erase(jobItr);
	graphicsJobsQueueMutex.unlock();
	return true;
}

bool Threading::getGPUTransferJob(JobBase *& job, s64& timeUntilNextJob)
{
	gpuTransferJobsQueueMutex.lock();
	if (gpuTransferJobs.empty())
	{
		gpuTransferJobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = gpuTransferJobs.begin();
	timeUntilNextJob = std::max((s64)0, (*jobItr)->getScheduledTime() - (s64)Engine::clock.now());
	while (timeUntilNextJob != 0)
	{
		++jobItr;
		if (jobItr == gpuTransferJobs.end())
		{
			gpuTransferJobsQueueMutex.unlock();
			return false;
		}
		timeUntilNextJob = std::max((s64)0, std::min((*jobItr)->getScheduledTime() - (s64)Engine::clock.now(), timeUntilNextJob));
	}
	job = *jobItr;
	gpuTransferJobs.erase(jobItr);
	gpuTransferJobsQueueMutex.unlock();
	return true;
}

void Threading::update()
{
	Engine::waitForProfilerInitMutex.lock();
	Engine::waitForProfilerInitMutex.unlock();
	initThreadProfilerTagsMutex.lock();
	Profiler::prealloc(std::vector<std::thread::id>{ std::this_thread::get_id() }, std::vector<std::string>{ "thread_" + getThisThreadIDString() });
	initThreadProfilerTagsMutex.unlock();

	threadJobsProcessed[std::this_thread::get_id()] = 0;
	
	Engine::renderer->createPerThreadCommandPools();
	JobBase* job;
	bool readyToTerminate = false;
	s64 timeUntilNextJob;
	while (!readyToTerminate)
	{
		PROFILE_START("thread_" + getThisThreadIDString());
		if (getJob(job, timeUntilNextJob))
		{
			++threadJobsProcessed[std::this_thread::get_id()];
			job->run();
		}
		else
		{
			readyToTerminate = !Engine::engineRunning;
			//std::this_thread::sleep_for(std::chrono::milliseconds(0));
			if (timeUntilNextJob > 10'000)
				std::this_thread::yield();
		}
		PROFILE_END("thread_" + getThisThreadIDString());
	}
}

void Threading::updateGraphics()
{
	Engine::waitForProfilerInitMutex.lock();
	Engine::waitForProfilerInitMutex.unlock();
	initThreadProfilerTagsMutex.lock();
	Profiler::prealloc(std::vector<std::thread::id>{ std::this_thread::get_id() }, std::vector<std::string>{ "thread_" + getThisThreadIDString() });
	initThreadProfilerTagsMutex.unlock();

	threadJobsProcessed[std::this_thread::get_id()] = 0;

	Engine::renderer->createPerThreadCommandPools();
	JobBase* job;
	bool readyToTerminate = false;
	s64 timeUntilNextJob;
	while (!readyToTerminate)
	{
		PROFILE_START("thread_" + getThisThreadIDString());
		if (getGraphicsJob(job, timeUntilNextJob)) // Graphics submission jobs have priority (they should be very fast)
		{
			++threadJobsProcessed[std::this_thread::get_id()];
			job->run();
		}
		//else if (getJob(job))
		//{
		//	++threadJobsProcessed[std::this_thread::get_id()];
		//	job->run();
		//}
		else
		{
			readyToTerminate = !Engine::engineRunning;
			//std::this_thread::sleep_for(std::chrono::milliseconds(0));
			if (timeUntilNextJob > 10'000)
				std::this_thread::yield();
		}
		PROFILE_END("thread_" + getThisThreadIDString());
	}
}

bool Threading::allRegularJobsDone()
{
	return totalJobsAdded.load() == totalJobsFinished.load();
}

bool Threading::allTransferJobsDone()
{
	return totalTransferJobsAdded.load() == totalTransferJobsFinished.load();
}

bool Threading::allAsyncJobsDone()
{
	return totalAsyncJobsAdded.load() == totalAsyncJobsFinished.load();
}

bool Threading::allGraphicsJobsDone()
{
	return totalGraphicsJobsAdded.load() == totalGraphicsJobsFinished.load();
}

void Threading::freeJob(JobBase * jobToFree)
{
	jobsFreeMutex.lock();
	jobsToFree.push_front(jobToFree);
	jobsFreeMutex.unlock();
}

void Threading::cleanupJobs()
{
	jobsFreeMutex.lock();
	for (auto j : jobsToFree)
	{
		delete j;
	}
	jobsToFree.clear();
	jobsFreeMutex.unlock();
}

void Threading::debugResetJobCounters()
{
	totalAsyncJobsAdded = 0;
	totalAsyncJobsFinished = 0;
	totalGraphicsJobsAdded = 0;
	totalGraphicsJobsFinished = 0;
	totalJobsAdded = 0;
	totalJobsFinished = 0;
	totalTransferJobsAdded = 0;
	totalTransferJobsFinished = 0;
}

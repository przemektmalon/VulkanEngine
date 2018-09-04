#include "PCH.hpp"
#include "Threading.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Profiler.hpp"

Threading::Threading(int pNumThreads) : workers(pNumThreads)
{
	totalCPUJobsAdded = 0;
	totalCPUJobsFinished = 0;
	totalGPUTransferJobsAdded = 0;
	totalGPUTransferJobsFinished = 0;
	totalCPUTransferJobsAdded = 0;
	totalCPUTransferJobsFinished = 0;
	totalGPUJobsAdded = 0;
	totalGPUJobsFinished = 0;
	for (int i = 0; i < pNumThreads - 2; ++i)
	{
		workers[i] = new std::thread(&Threading::update, this);
	}
	workers[pNumThreads - 1] = new std::thread(&Threading::updateGraphics, this);
	workers[pNumThreads - 2] = new std::thread(&Threading::updateCPUTransfers, this);
}

Threading::~Threading()
{
	/// TODO: notify threads to force terminate ??
	for (auto w : workers)
	{
		w->join();
	}
}

void Threading::addCPUJob(JobBase * jobToAdd)
{
	cpuJobsQueueMutex.lock();
	totalCPUJobsAdded += 1;
	cpuJobs.push_back(jobToAdd);
	cpuJobsQueueMutex.unlock();
}

void Threading::addGPUJob(JobBase * jobToAdd)
{
	gpuJobsQueueMutex.lock();
	totalGPUJobsAdded += 1;
	gpuJobs.push_back(jobToAdd);
	gpuJobsQueueMutex.unlock();
}

void Threading::addGPUTransferJob(JobBase * jobToAdd)
{
	gpuTransferJobsQueueMutex.lock();
	totalGPUTransferJobsAdded += 1;
	gpuTransferJobs.push_back(jobToAdd);
	gpuTransferJobsQueueMutex.unlock();
}

void Threading::addCPUTransferJob(JobBase * jobToAdd)
{
	cpuTransferJobsQueueMutex.lock();
	totalCPUTransferJobsAdded += 1;
	cpuTransferJobs.push_back(jobToAdd);
	cpuTransferJobsQueueMutex.unlock();
}

bool Threading::getCPUJob(JobBase *& job, s64& timeUntilNextJob)
{
	cpuJobsQueueMutex.lock();
	if (cpuJobs.empty())
	{
		cpuJobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = cpuJobs.begin();
	timeUntilNextJob = std::max((s64)0, (*jobItr)->getScheduledTime() - (s64)Engine::clock.now());
	while (timeUntilNextJob != 0)
	{
		++jobItr;
		if (jobItr == cpuJobs.end())
		{
			cpuJobsQueueMutex.unlock();
			return false;
		}
		timeUntilNextJob = std::max((s64)0, std::min((*jobItr)->getScheduledTime() - (s64)Engine::clock.now(), timeUntilNextJob));
	}
	job = *jobItr;
	cpuJobs.erase(jobItr);
	cpuJobsQueueMutex.unlock();
	return true;
}

bool Threading::getGPUJob(JobBase *& job, s64& timeUntilNextJob)
{
	gpuJobsQueueMutex.lock();
	if (gpuJobs.empty())
	{
		gpuJobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = gpuJobs.begin();
	timeUntilNextJob = std::max((s64)0, (*jobItr)->getScheduledTime() - (s64)Engine::clock.now());
	while (timeUntilNextJob > 0)
	{
		++jobItr;
		if (jobItr == gpuJobs.end())
		{
			gpuJobsQueueMutex.unlock();
			return false;
		}
		timeUntilNextJob = std::max((s64)0, std::min((*jobItr)->getScheduledTime() - (s64)Engine::clock.now(), timeUntilNextJob));
	}
	job = *jobItr;
	gpuJobs.erase(jobItr);
	gpuJobsQueueMutex.unlock();
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

bool Threading::getCPUTransferJob(JobBase *& job, s64 & timeUntilNextJob)
{
	cpuTransferJobsQueueMutex.lock();
	if (cpuTransferJobs.empty())
	{
		cpuTransferJobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = cpuTransferJobs.begin();
	timeUntilNextJob = std::max((s64)0, (*jobItr)->getScheduledTime() - (s64)Engine::clock.now());
	while (timeUntilNextJob != 0)
	{
		++jobItr;
		if (jobItr == cpuTransferJobs.end())
		{
			cpuTransferJobsQueueMutex.unlock();
			return false;
		}
		timeUntilNextJob = std::max((s64)0, std::min((*jobItr)->getScheduledTime() - (s64)Engine::clock.now(), timeUntilNextJob));
	}
	job = *jobItr;
	cpuTransferJobs.erase(jobItr);
	cpuTransferJobsQueueMutex.unlock();
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
		if (getCPUJob(job, timeUntilNextJob))
		{
			++threadJobsProcessed[std::this_thread::get_id()];
			job->run();
		}
		else
		{
			readyToTerminate = !Engine::engineRunning;
			if (timeUntilNextJob > 10'000)
				std::this_thread::yield();
		}
		PROFILE_END("thread_" + getThisThreadIDString());
	}

	while (!allGraphicsJobsDone())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	vkQueueWaitIdle(Engine::renderer->lGraphicsQueue.getHandle());
	vkQueueWaitIdle(Engine::renderer->lTransferQueue.getHandle());
	Engine::renderer->commandPool.destroy();
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
		if (getGPUJob(job, timeUntilNextJob)) // Graphics submission jobs have priority (they should be very fast)
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
			if (timeUntilNextJob > 10'000)
				std::this_thread::yield();
		}
		PROFILE_END("thread_" + getThisThreadIDString());
	}

	while (!allGraphicsJobsDone())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	Engine::renderer->executeFenceDelayedActions();
	Engine::renderer->lGraphicsQueue.waitIdle();
	Engine::renderer->lTransferQueue.waitIdle();
	Engine::renderer->commandPool.destroy();
}

void Threading::updateCPUTransfers()
{
	Engine::waitForProfilerInitMutex.lock();
	Engine::waitForProfilerInitMutex.unlock();
	initThreadProfilerTagsMutex.lock();
	Profiler::prealloc(std::vector<std::thread::id>{ std::this_thread::get_id() }, std::vector<std::string>{ "thread_" + getThisThreadIDString() });
	initThreadProfilerTagsMutex.unlock();

	threadJobsProcessed[std::this_thread::get_id()] = 0;

	JobBase* job;
	bool readyToTerminate = false;
	s64 timeUntilNextJob;
	while (!readyToTerminate)
	{
		PROFILE_START("thread_" + getThisThreadIDString());
		if (getCPUTransferJob(job, timeUntilNextJob))
		{
			++threadJobsProcessed[std::this_thread::get_id()];
			job->run();
		}
		else
		{
			readyToTerminate = !Engine::engineRunning;
			if (timeUntilNextJob > 10'000)
				std::this_thread::yield();
		}
		PROFILE_END("thread_" + getThisThreadIDString());
	}
}

bool Threading::allNonGPUJobsDone()
{
	return totalCPUJobsAdded == totalCPUJobsFinished && totalCPUTransferJobsAdded == totalCPUTransferJobsFinished;
}

bool Threading::allGPUTransferJobsDone()
{
	return totalGPUTransferJobsAdded == totalGPUTransferJobsFinished;
}

bool Threading::allGraphicsJobsDone()
{
	return totalGPUJobsAdded == totalGPUJobsFinished;
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


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
	jobs.push_front(jobToAdd);
	jobsQueueMutex.unlock();
}

void Threading::addGraphicsJob(JobBase * jobToAdd, int async)
{
	graphicsJobsQueueMutex.lock();
	totalGraphicsJobsAdded.fetch_add(1 - async);
	totalAsyncJobsAdded.fetch_add(async);
	graphicsJobs.push_front(jobToAdd);
	graphicsJobsQueueMutex.unlock();
}

void Threading::addGPUTransferJob(JobBase * jobToAdd, int async)
{
	gpuTransferJobsQueueMutex.lock();
	totalTransferJobsAdded.fetch_add(1 - async);
	totalAsyncJobsAdded.fetch_add(async);
	gpuTransferJobs.push_front(jobToAdd);
	gpuTransferJobsQueueMutex.unlock();
}

bool Threading::getJob(JobBase *& job)
{
	jobsQueueMutex.lock();
	if (jobs.empty())
	{
		jobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = jobs.begin();
	auto prevJob = jobs.before_begin();
	while ((*jobItr)->getScheduledTime() > Engine::clock.now())
	{
		++prevJob;
		++jobItr;
		if (jobItr == jobs.end())
			return false;
	}
	job = *jobItr;
	jobs.erase_after(prevJob);
	jobsQueueMutex.unlock();
	return true;
}

bool Threading::getGraphicsJob(JobBase *& job)
{
	graphicsJobsQueueMutex.lock();
	if (graphicsJobs.empty())
	{
		graphicsJobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = graphicsJobs.begin();
	auto prevJob = graphicsJobs.before_begin();
	while ((*jobItr)->getScheduledTime() > Engine::clock.now())
	{
		++prevJob;
		++jobItr;
		if (jobItr == graphicsJobs.end())
			return false;
	}
	job = *jobItr;
	graphicsJobs.erase_after(prevJob);
	graphicsJobsQueueMutex.unlock();
	return true;
}

bool Threading::getGPUTransferJob(JobBase *& job)
{
	gpuTransferJobsQueueMutex.lock();
	if (gpuTransferJobs.empty())
	{
		gpuTransferJobsQueueMutex.unlock();
		return false;
	}
	auto jobItr = gpuTransferJobs.begin();
	auto prevJob = gpuTransferJobs.before_begin();
	while ((*jobItr)->getScheduledTime() > Engine::clock.now())
	{
		++prevJob;
		++jobItr;
		if (jobItr == gpuTransferJobs.end())
			return false;
	}
	job = *jobItr;
	gpuTransferJobs.erase_after(prevJob);
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
	while (!readyToTerminate)
	{
		PROFILE_START("thread_" + getThisThreadIDString());
		if (getJob(job))
		{
			++threadJobsProcessed[std::this_thread::get_id()];
			job->run();
		}
		else
		{
			readyToTerminate = !Engine::engineRunning;
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
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
	while (!readyToTerminate)
	{
		PROFILE_START("thread_" + getThisThreadIDString());
		if (getGraphicsJob(job)) // Graphics submission jobs have priority (they should be very fast)
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
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
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

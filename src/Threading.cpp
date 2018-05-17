#include "PCH.hpp"
#include "Threading.hpp"

Threading::Threading(int pNumThreads) : workers(pNumThreads)
{
	totalJobsAdded.store(0);
	totalJobsFinished.store(0);
	for (int i = 0; i < pNumThreads - 1; ++i)
	{
		workers[i] = new std::thread(&Threading::update, this);
	}
	workers[pNumThreads - 1] = new std::thread(&Threading::updateGraphics, this);
}

Threading::~Threading()
{
	/// TODO: notify threads to force terminate ??
	for (auto w : workers)
	{
		w->join();
	}
}

void Threading::addJob(JobBase * jobToAdd)
{
	jobsQueueMutex.lock();
	totalJobsAdded.fetch_add(1);
	jobs.emplace(jobToAdd);
	jobsQueueMutex.unlock();
}

void Threading::addGraphicsJob(JobBase * jobToAdd)
{
	graphicsJobsQueueMutex.lock();
	totalJobsAdded.fetch_add(1);
	graphicsJobs.emplace(jobToAdd);
	graphicsJobsQueueMutex.unlock();
}

void Threading::addGPUTransferJob(JobBase * jobToAdd)
{
	gpuTransferJobsQueueMutex.lock();
	totalTransferJobsAdded.fetch_add(1);
	gpuTransferJobs.emplace(jobToAdd);
	gpuTransferJobsQueueMutex.unlock();
}

bool Threading::getJob(JobBase *& job)
{
	jobsQueueMutex.lock();
	if (jobs.size() == 0)
	{
		jobsQueueMutex.unlock();
		return false;
	}
	job = jobs.front();
	jobs.pop();
	jobsQueueMutex.unlock();
	return true;
}

bool Threading::getGraphicsJob(JobBase *& job)
{
	graphicsJobsQueueMutex.lock();
	if (graphicsJobs.size() == 0)
	{
		graphicsJobsQueueMutex.unlock();
		return false;
	}
	job = graphicsJobs.front();
	graphicsJobs.pop();
	graphicsJobsQueueMutex.unlock();
	return true;
}

bool Threading::getGPUTransferJob(JobBase *& job)
{
	gpuTransferJobsQueueMutex.lock();
	if (gpuTransferJobs.size() == 0)
	{
		gpuTransferJobsQueueMutex.unlock();
		return false;
	}
	job = gpuTransferJobs.front();
	gpuTransferJobs.pop();
	gpuTransferJobsQueueMutex.unlock();
	return true;
}

void Threading::update()
{
	JobBase* job;
	while (true)
	{
		if (getJob(job))
			job->run();
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Threading::updateGraphics()
{
	JobBase* job;
	while (true)
	{
		if (getGraphicsJob(job))
			job->run();
		else if (getJob(job))
			job->run();
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

bool Threading::allJobsDone()
{
	return totalJobsAdded.load() == totalJobsFinished.load();
}

bool Threading::allTransferJobsDone()
{
	return totalTransferJobsAdded.load() == totalTransferJobsFinished.load();
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

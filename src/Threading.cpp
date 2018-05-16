#include "PCH.hpp"
#include "Threading.hpp"

Threading::Threading(int pNumThreads) : workers(pNumThreads)
{
	totalJobsAdded.store(0);
	totalJobsFinished.store(0);
	for (auto w : workers)
	{
		w = new std::thread(&Threading::update, this);
	}
}

Threading::~Threading()
{
	/// TODO: notify threads to terminate
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

bool Threading::allJobsDone()
{
	return totalJobsAdded.load() == totalJobsFinished.load();
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

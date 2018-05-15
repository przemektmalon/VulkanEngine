#pragma once
#include "PCH.hpp"

// Job -- function to call with some arguments (by some thread)
// Threading -- worker threads and job queue

class Job
{
public:
	Job() {}
	Job(std::function<void(void)> pJobFunction, std::function<void(void)> pDoneFunction) : jobFunction(pJobFunction), doneFunction(pDoneFunction) {}

	std::function<void(void)> jobFunction;
	std::function<void(void)> doneFunction;

	void run() { jobFunction(); doneFunction(); }

private:


};

class Threading
{
public:

	// Construct worker threads
	Threading(int pNumThreads) : workers(pNumThreads)
	{
		totalJobsAdded.store(0);
		totalJobsFinished.store(0);
		for (auto w : workers)
		{
			w = new std::thread(&Threading::update, this);
		}
	}

	// Terminate worker threads
	~Threading()
	{
		/// TODO: notify threads to terminate
		for (auto w : workers)
		{
			w->join();
		}
	}

	// Systems add their jobs to the queue
	void addJob(Job& jobToAdd)
	{
		jobsQueueMutex.lock();
		totalJobsAdded.fetch_add(1);
		jobs.emplace(jobToAdd);
		jobsQueueMutex.unlock();
	}

	// Each worker will grab some job
	bool getJob(Job& job)
	{	
		jobsQueueMutex.lock();
		if (jobs.size() == 0)
		{
			jobsQueueMutex.unlock();
			return false;
			/*return Job([]()->void {
				//std::cout << "No jobs for thread: " << std::this_thread::get_id() << " sleeping" << std::endl;
				//std::this_thread::sleep_for(std::chrono::microseconds(1));
			}, []()->void {});*/
		}
		job = jobs.front();
		jobs.pop();
		jobsQueueMutex.unlock();
		return true;
	}

	// Executed by each worker thread
	void update()
	{
		Job job;
		while (true)
		{
			if (getJob(job))
				job.run();
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}


	std::mutex jobsQueueMutex;
	std::queue<Job> jobs;
	std::atomic_char32_t totalJobsAdded;
	std::atomic_char32_t totalJobsFinished;
	std::vector<std::thread*> workers;


	// Testing mutexes

	std::mutex renderMutex;
	std::mutex physBulletMutex;
	std::mutex physToEngineMutex;
	std::mutex physToGPUMutex;
};
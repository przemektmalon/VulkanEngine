#pragma once
#include "PCH.hpp"
#include "Engine.hpp"

// Job -- function to call with some arguments (by some thread)
// Threading -- worker threads and job queue

class JobBase
{
public:
	JobBase() {};

	virtual void run() = 0;
};

typedef std::function<void(void)> VoidJobType;

template<typename JobFuncType = std::function<void(void)>, typename DoneFuncType = std::function<void(void)>>
class Job : public JobBase
{
public:
	Job() {}
	Job(JobFuncType pJobFunction) : jobFunction(pJobFunction), doneFunction([]()->void {}) {}
	Job(JobFuncType pJobFunction, DoneFuncType pDoneFunction) : jobFunction(pJobFunction), doneFunction(pDoneFunction) {}

	void run()
	{
		jobFunction();
		doneFunction();
		Engine::threading->freeJob(this);
	}

private:

	JobFuncType jobFunction;
	DoneFuncType doneFunction;
};

class Threading
{
public:

	// Construct worker threads
	Threading(int pNumThreads);

	// Terminate worker threads
	~Threading();

	// Systems add their jobs to the queue
	void addJob(JobBase* jobToAdd);

	// Each worker will grab some job
	bool getJob(JobBase*& job);

	// Executed by each worker thread
	void update();

	// Are all jobs done
	bool allJobsDone();

	// Mark job for freeing memory
	void freeJob(JobBase* jobToFree);

	// Free marked jobs
	void cleanupJobs();

	std::mutex jobsQueueMutex;
	std::mutex jobsFreeMutex;
	std::queue<JobBase*> jobs;
	std::list<JobBase*> jobsToFree;
	std::atomic_char32_t totalJobsAdded;
	std::atomic_char32_t totalJobsFinished;
	std::vector<std::thread*> workers;

	// Testing mutexes

	std::mutex renderMutex;
	std::mutex physBulletMutex;
	std::mutex physToEngineMutex;
	std::mutex physToGPUMutex;
};

static void defaultJobDoneFunc()
{
	Engine::threading->totalJobsFinished.fetch_add(1);
}
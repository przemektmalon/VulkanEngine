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

template<typename JobFuncType = VoidJobType, typename DoneFuncType = VoidJobType>
class Job : public JobBase
{
public:
	Job() {}
	Job(JobFuncType pJobFunction) : jobFunction(pJobFunction), doneFunction([]()->void {}), child(nullptr) {}
	Job(JobFuncType pJobFunction, DoneFuncType pDoneFunction) : jobFunction(pJobFunction), doneFunction(pDoneFunction), child(nullptr) {}

	void run()
	{
		jobFunction();
		if (child)
			Engine::threading->addGraphicsJob(child); /// TODO: might not be a graphics job
		doneFunction();
		Engine::threading->freeJob(this);
	}

	void setChild(JobBase* pChild)
	{
		child = pChild;
	}

private:

	JobBase * child;
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
	
	// Vulkan queue submissions can only be done from one thread
	void addGraphicsJob(JobBase* jobToAdd);

	// Each worker will grab some job
	bool getJob(JobBase*& job);

	// Graphics submission thread will take these jobs before regular ones
	bool getGraphicsJob(JobBase*& job);

	// Executed by each worker thread
	void update();

	// Executed by graphics submission thread before regular update
	void updateGraphics();

	// Are all jobs done
	bool allJobsDone();

	// Mark job for freeing memory
	void freeJob(JobBase* jobToFree);

	// Free marked jobs
	void cleanupJobs();

	std::mutex jobsQueueMutex;
	std::mutex graphicsJobsQueueMutex;
	std::mutex jobsFreeMutex;
	std::queue<JobBase*> jobs;
	std::queue<JobBase*> graphicsJobs;
	std::list<JobBase*> jobsToFree;
	std::atomic_char32_t totalJobsAdded;
	std::atomic_char32_t totalJobsFinished;
	std::vector<std::thread*> workers;

	// Testing mutexes

	std::mutex renderMutex;
	std::mutex physBulletMutex;
	std::mutex physToEngineMutex;
	std::mutex physToGPUMutex;

	std::mutex instanceTransformMutex;
	std::mutex addingModelInstanceMutex;
	std::mutex pushingModelToGPUMutex;
};

static void defaultJobDoneFunc()
{
	Engine::threading->totalJobsFinished.fetch_add(1);
}
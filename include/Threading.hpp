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
			Engine::threading->addGPUTransferJob(child); /// TODO: Might not be a transfer job, for now all are ( asset to gpu transfers )
														 ///       Find an efficient solution to adding any type of job as child ( or some other solution )
														 ///	   Not priority unless we start adding a lot of heavy child jobs that will hog the main thread
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
	
	// Vulkan queue submissions can only be done from one thread (per queue)
	void addGraphicsJob(JobBase* jobToAdd);

	// During rendering GPU memory transfer operation will be submitted to a separate transfer queue from the main thread
	void addGPUTransferJob(JobBase* jobToAdd);

	// Each worker will grab some job
	bool getJob(JobBase*& job);

	// Graphics submission thread will take these jobs before regular ones
	bool getGraphicsJob(JobBase*& job);

	// The main thread will grab these jobs each iteration of its loop and send them to the GPU
	bool getGPUTransferJob(JobBase*& job);


	// Thread_locals with special construction are initialised in the 'update' (thread entry) functions

	// Executed by each worker thread 
	void update();

	// Executed by graphics submission thread before regular update
	void updateGraphics();

	// Are all (non-transfer) jobs done
	bool allJobsDone();

	// Are all transfer jobs done
	bool allTransferJobsDone();

	// Mark job for freeing memory
	void freeJob(JobBase* jobToFree);

	// Free marked jobs
	void cleanupJobs();

	std::queue<JobBase*> jobs;
	std::mutex jobsQueueMutex;

	std::queue<JobBase*> graphicsJobs;
	std::mutex graphicsJobsQueueMutex;
	
	std::queue<JobBase*> gpuTransferJobs;
	std::mutex gpuTransferJobsQueueMutex;

	std::list<JobBase*> jobsToFree;
	std::mutex jobsFreeMutex;

	std::atomic_char32_t totalJobsAdded;
	std::atomic_char32_t totalJobsFinished;

	std::atomic_char32_t totalTransferJobsAdded;
	std::atomic_char32_t totalTransferJobsFinished;

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

static void defaultTransferJobDoneFunc()
{
	Engine::threading->totalTransferJobsFinished.fetch_add(1);
}
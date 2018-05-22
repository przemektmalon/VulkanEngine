#pragma once
#include "PCH.hpp"
#include "Engine.hpp"

// Job -- function to call with some arguments (by some thread)
// Threading -- worker threads and job queue

class JobBase
{
public:
	enum Type { Regular, Graphics, Transfer };

	JobBase(Type pType) : type(pType), child(nullptr) {}

	virtual void run() = 0;

	void setChild(JobBase* pChild)
	{
		child = pChild;
	}

	void setType(Type pType)
	{
		type = pType;
	}

	Type getType()
	{
		return type;
	}

protected:

	JobBase * child;
	Type type;
};

typedef std::function<void(void)> VoidJobType;

template<typename JobFuncType = VoidJobType, typename DoneFuncType = VoidJobType>
class Job : public JobBase
{
public:
	Job(JobFuncType pJobFunction, DoneFuncType pDoneFunction) : jobFunction(pJobFunction), doneFunction(pDoneFunction), JobBase(Regular) {}
	Job(JobFuncType pJobFunction, DoneFuncType pDoneFunction, Type pType) : jobFunction(pJobFunction), doneFunction(pDoneFunction), JobBase(pType) {}

	void run()
	{
		jobFunction();
		if (child)
		{
			switch (child->getType())
			{
			case (Regular):
				Engine::threading->addJob(child);
				break;
			case (Graphics):
				Engine::threading->addGraphicsJob(child);
				break;
			case (Transfer):
				Engine::threading->addGPUTransferJob(child);
				break;
			}
		}	
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

	// 'async' jobs are not counted in job total and finish counters so as to not block on frame borders

	// Systems add their jobs to the queue
	void addJob(JobBase* jobToAdd, int async = 0);
	
	// Vulkan queue submissions can only be done from one thread (per queue)
	void addGraphicsJob(JobBase* jobToAdd, int async = 0);

	// During rendering GPU memory transfer operation will be submitted to a separate transfer queue from the main thread
	void addGPUTransferJob(JobBase* jobToAdd, int async = 0);

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

	std::mutex physBulletMutex;
	std::mutex physToEngineMutex;
	std::mutex physToGPUMutex;

	std::mutex instanceTransformMutex;
	std::mutex addingModelInstanceMutex;
	std::mutex pushingModelToGPUMutex;

	std::mutex layersMutex;
};

static void defaultJobDoneFunc()
{
	Engine::threading->totalJobsFinished.fetch_add(1);
}

static void defaultTransferJobDoneFunc()
{
	Engine::threading->totalTransferJobsFinished.fetch_add(1);
}

static void defaultAsyncJobDoneFunc()
{
}
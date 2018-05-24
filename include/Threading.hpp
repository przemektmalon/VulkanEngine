#pragma once
#include "PCH.hpp"
#include "Engine.hpp"

// Job -- function to call with some arguments (by some thread)
// Threading -- worker threads and job queue

class JobBase
{
public:
	enum Type { Regular, Graphics, Transfer };

	JobBase(Type pType) : type(pType), child(nullptr), scheduledTime(0) {}
	JobBase(Type pType, u64 pScheduledTime) : type(pType), child(nullptr), scheduledTime(pScheduledTime) {}

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

	void setScheduledTime(u64 pScheduledTime)
	{
		scheduledTime = pScheduledTime;
	}

	s64 getScheduledTime()
	{
		return scheduledTime;
	}

protected:

	s64 scheduledTime;
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

	// Get thread ID strings
	static std::string getThisThreadIDString() 
	{ 
		std::stringstream ss;
		ss << std::this_thread::get_id();
		return ss.str();
	}
	static std::string getThreadIDString(std::thread::id id) 
	{ 
		std::stringstream ss;
		ss << id;
		return ss.str();
	}

	std::mutex initThreadProfilerTagsMutex;
	std::unordered_map<int, std::thread::id> threadIDAssociations;
	std::unordered_map<std::thread::id, int> threadJobsProcessed;

	// 'async' jobs are not counted in job total and finish counters so as to not block on frame borders

	// Systems add their jobs to the queue
	void addJob(JobBase* jobToAdd, int async = 0);
	
	// Vulkan queue submissions can only be done from one thread (per queue)
	void addGraphicsJob(JobBase* jobToAdd, int async = 0);

	// During rendering GPU memory transfer operation will be submitted to a separate transfer queue from the main thread
	void addGPUTransferJob(JobBase* jobToAdd, int async = 0);

	// Each worker will grab some job
	bool getJob(JobBase*& job, s64& timeUntilNextJob);

	// Graphics submission thread will take these jobs before regular ones
	bool getGraphicsJob(JobBase*& job, s64& timeUntilNextJob);

	// The main thread will grab these jobs each iteration of its loop and send them to the GPU
	bool getGPUTransferJob(JobBase*& job, s64& timeUntilNextJob);


	// Thread_locals with special construction are initialised in the 'update' (thread entry) functions

	// Executed by each worker thread 
	void update();

	// Executed by graphics submission thread before regular update
	void updateGraphics();

	// Are all regular jobs done
	bool allRegularJobsDone();

	// Are all transfer jobs done
	bool allTransferJobsDone();

	// Are all 'async' jobs done
	bool allAsyncJobsDone();

	// Are all graphics jobs done
	bool allGraphicsJobsDone();

	// Mark job for freeing memory
	void freeJob(JobBase* jobToFree);

	// Free marked jobs
	void cleanupJobs();

	// Sometimes (on console open close) some jobs seem to finish but not add to the counter, this resets job counters
	void debugResetJobCounters();

	std::list<JobBase*> jobs;
	std::mutex jobsQueueMutex;

	std::list<JobBase*> graphicsJobs;
	std::mutex graphicsJobsQueueMutex;
	
	std::list<JobBase*> gpuTransferJobs;
	std::mutex gpuTransferJobsQueueMutex;

	std::list<JobBase*> jobsToFree;
	std::mutex jobsFreeMutex;

	std::atomic_char32_t totalJobsAdded;
	std::atomic_char32_t totalJobsFinished;

	std::atomic_char32_t totalTransferJobsAdded;
	std::atomic_char32_t totalTransferJobsFinished;

	std::atomic_char32_t totalAsyncJobsAdded;
	std::atomic_char32_t totalAsyncJobsFinished;

	std::atomic_char32_t totalGraphicsJobsAdded;
	std::atomic_char32_t totalGraphicsJobsFinished;

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

static void defaultGraphicsJobDoneFunc()
{
	Engine::threading->totalGraphicsJobsFinished.fetch_add(1);
}

static void defaultTransferJobDoneFunc()
{
	Engine::threading->totalTransferJobsFinished.fetch_add(1);
}

static void defaultAsyncJobDoneFunc()
{
	Engine::threading->totalAsyncJobsFinished.fetch_add(1);
}
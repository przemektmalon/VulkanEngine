#pragma once
#include "PCH.hpp"
#include "Engine.hpp"

// Job -- function to call with some arguments (by some thread)
// Threading -- worker threads and job queue

class JobBase
{
public:
	enum Type { CPU, GPU, CPUTransfer, GPUTransfer };

protected:

	JobBase() = delete;
	JobBase(const JobBase&) = delete;
	JobBase(Type pType) : type(pType), child(nullptr), scheduledTime(0) {}
	JobBase(Type pType, u64 pScheduledTime) : type(pType), child(nullptr), scheduledTime(pScheduledTime) {}

public:

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

template<typename JobFuncType = VoidJobType>
class Job : public JobBase
{
public:
	Job(JobFuncType pJobFunction) : jobFunction(pJobFunction), JobBase(CPU) {}
	Job(JobFuncType pJobFunction, Type pType) : jobFunction(pJobFunction), JobBase(pType) {}

	void run()
	{
		jobFunction();
		if (child)
		{
			switch (child->getType())
			{
			case (CPU):
				Engine::threading->addCPUJob(child);
				break;
			case (GPU):
				Engine::threading->addGPUJob(child);
				break;
			case (CPUTransfer):
				Engine::threading->addCPUTransferJob(child);
				break;
			case (GPUTransfer):
				Engine::threading->addGPUTransferJob(child);
				break;
			}
		}	

		switch (type)
		{
			case (CPU):
				Engine::threading->totalCPUJobsFinished.fetch_add(1);
				break;
			case (GPU):
				Engine::threading->totalGPUJobsFinished.fetch_add(1);
				break;
			case (CPUTransfer):
				Engine::threading->totalCPUTransferJobsFinished.fetch_add(1);
				break;
			case (GPUTransfer):
				Engine::threading->totalGPUTransferJobsFinished.fetch_add(1);
				break;
		}

		Engine::threading->freeJob(this);
	}

private:

	JobFuncType jobFunction;
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

	// Systems add their jobs to the queue, jobs submitted with this function CANNOT submit GPU operations
	void addCPUJob(JobBase* jobToAdd);
	
	// Vulkan queue submissions can only be done from one thread (each queue must have its own thread)
	void addGPUJob(JobBase* jobToAdd);

	// During rendering CPU to GPU memory transfer operation will be submitted to a separate transfer queue from the main thread
	void addGPUTransferJob(JobBase* jobToAdd);

	// DISK <-> RAM transfer operation will have their own thread
	void addCPUTransferJob(JobBase* jobToAdd);

	// Each worker will grab the next job
	bool getCPUJob(JobBase*& job, s64& timeUntilNextJob);

	// Graphics submission thread will prioritise GPU jobs
	bool getGPUJob(JobBase*& job, s64& timeUntilNextJob);

	// The main thread will grab these jobs each iteration of its loop and send them to the GPU tranfers queue
	bool getGPUTransferJob(JobBase*& job, s64& timeUntilNextJob);

	// The DISK thread will grab these jobs
	bool getCPUTransferJob(JobBase*& job, s64& timeUntilNextJob);

	// Executed by each worker thread 
	// Thread_locals with special construction are initialised here
	void update();

	// Executed by graphics submission thread before regular update
	void updateGraphics();

	// Executed by DISK IO thread
	void updateCPUTransfers();

	// Are all regular jobs done
	bool allNonGPUJobsDone();

	// Are all transfer jobs done
	bool allGPUTransferJobsDone();

	// Are all graphics jobs done
	bool allGPUJobsDone();

	// Mark job for freeing memory
	void freeJob(JobBase* jobToFree);

	// Free marked jobs
	void cleanupJobs();

	std::list<JobBase*> cpuJobs;
	std::mutex cpuJobsQueueMutex;

	std::list<JobBase*> gpuJobs;
	std::mutex gpuJobsQueueMutex;
	
	std::list<JobBase*> gpuTransferJobs;
	std::mutex gpuTransferJobsQueueMutex;

	std::list<JobBase*> cpuTransferJobs;
	std::mutex cpuTransferJobsQueueMutex;

	std::list<JobBase*> jobsToFree;
	std::mutex jobsFreeMutex;

	std::atomic_char32_t totalCPUJobsAdded;
	std::atomic_char32_t totalCPUJobsFinished;

	std::atomic_char32_t totalGPUJobsAdded;
	std::atomic_char32_t totalGPUJobsFinished;

	std::atomic_char32_t totalGPUTransferJobsAdded;
	std::atomic_char32_t totalGPUTransferJobsFinished;

	std::atomic_char32_t totalCPUTransferJobsAdded;
	std::atomic_char32_t totalCPUTransferJobsFinished;

	std::vector<std::thread*> workers;

	// Engine mutexes
	// Maybe another place for this ? A map of mutexes ?

	std::mutex physBulletMutex;
	std::mutex physToEngineMutex;
	std::mutex physToGPUMutex;
	std::mutex physObjectAddMutex;

	std::mutex instanceTransformMutex;
	std::mutex addingModelInstanceMutex;
	std::mutex pushingModelToGPUMutex;

	std::mutex addMaterialMutex;

	std::mutex layersMutex;
};
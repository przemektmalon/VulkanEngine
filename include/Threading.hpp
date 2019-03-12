#pragma once
#include "PCH.hpp"
#include "Engine.hpp"
#include "Profiler.hpp"

// class Threading	-- worker threads with job queues
// class Job		-- function to call with some arguments (by some worker thread)

class JobBase;

class Threading
{
	friend class JobBase;
	class WorkerThread {
	public:
		WorkerThread(std::function<void(void)> initFunc, std::function<void(void)> closeFunc, const std::string& name) :
			m_initFunc(initFunc), m_closeFunc(closeFunc), m_name(name), m_thread(&WorkerThread::run, this), m_totalJobsAdded(0), m_totalJobsFinished(0) {}
		WorkerThread(const WorkerThread&) = delete;
		WorkerThread& operator=(const WorkerThread&) = delete;

		void pushJob(JobBase* job);
		bool popJob(JobBase*& job);

		bool allJobsFinished() { return m_totalJobsAdded == m_totalJobsFinished; }
		void waitForAllJobsToFinish() {
			while (!allJobsFinished())
				std::this_thread::yield();
		}

		void join() { m_thread.join(); }

		std::thread::id getID() { return m_thread.get_id(); }

	private:

		void run();

		std::thread m_thread;

		std::queue<JobBase*> m_jobsQueue;
		std::mutex m_jobsQueueMutex;

		std::atomic_char32_t m_totalJobsAdded;
		std::atomic_char32_t m_totalJobsFinished;

		std::function<void(void)> m_initFunc;
		std::function<void(void)> m_closeFunc;
		std::string m_name;
	};

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

	// Compulsory workers (CPU, DISK I/O, GPU)
	WorkerThread* cpuWorker;
	WorkerThread* diskIOWorker;
	WorkerThread* gpuWorker;

	std::mutex initThreadProfilerTagsMutex;
	std::unordered_map<int, std::thread::id> threadIDAssociations;
	
	void addWorkerThread(const std::function<void(void)>& initFunc, const std::function<void(void)>& closeFunc, const std::string& name) {
		workerThreads.push_back(new WorkerThread(initFunc, closeFunc, name));
	}

	std::vector<WorkerThread*> workerThreads;

	// Systems add their jobs to the queue, jobs submitted with this function CANNOT submit GPU operations
	void addCPUJob(JobBase* job);

	// Vulkan queue submissions can only be done from one thread (each queue must have its own thread)
	void addGPUJob(JobBase* job);

	/// {{During rendering CPU to GPU memory transfer operation will be submitted to a separate transfer queue from the main thread}}
	/// NOTE: for now this is run on the GPU worker thread!!
	/// TODO: consider if there is any benefit in this running on the main thread
	/// void addGPUTransferJob(JobBase* job);

	// DISK <-> RAM transfer operation will have their own thread
	void addCPUTransferJob(JobBase* job);

	// Mark job for freeing memory
	void freeJob(JobBase* job);

	// Free jobs that are finished
	void cleanupJobs();

	std::list<JobBase*> jobsToFree;
	std::mutex jobsFreeMutex;


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

private:

	// Initialise compulsory threads 
	void initCompulsoryWorkers();
};

class JobBase
{
	friend class Threading;
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

	Threading::WorkerThread* owningWorker;
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
				//Engine::threading->addGPUTransferJob(child);
				Engine::threading->addGPUJob(child);
				break;
			}
		}

		Engine::threading->freeJob(this);
	}

private:

	JobFuncType jobFunction;
};




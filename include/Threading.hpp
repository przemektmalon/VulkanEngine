#pragma once
#include "PCH.hpp"
#include "Engine.hpp"
#include "Profiler.hpp"

// class Threading	-- worker threads with job queues
// class Job		-- function to call with some arguments (by some worker thread)

typedef std::function<void(void)> VoidJobType;

class JobBase;
template<typename>
class Job;

class Threading
{
	friend class JobBase;
	template<typename> friend class Job;
	

protected:

	class WorkerThread {
	public:
		WorkerThread(VoidJobType initFunc, VoidJobType closeFunc, const std::string& name) :
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

		VoidJobType m_initFunc;
		VoidJobType m_closeFunc;
		std::string m_name;
	};

public:

	// Construct worker threads
	Threading(int pNumThreads);

	// Terminate worker threads
	~Threading();

	// Get thread IDs as strings
	static std::string getThisThreadIDString();
	static std::string getThreadIDString(std::thread::id id);

	std::mutex m_initThreadProfilerTagsMutex;
	std::unordered_map<int, std::thread::id> m_threadIDAssociations;
	
	// Compulsory workers (CPU, DISK I/O, GPU)
	WorkerThread* m_cpuWorker;
	WorkerThread* m_diskIOWorker;
	WorkerThread* m_gpuWorker;

	void addWorkerThread(const std::function<void(void)>& initFunc, const std::function<void(void)>& closeFunc, const std::string& name) {
		m_workerThreads.push_back(new WorkerThread(initFunc, closeFunc, name));
	}

	std::vector<WorkerThread*> m_workerThreads;

	// Systems add their jobs to the queue, jobs submitted with this function CANNOT submit GPU operations
	void addCPUJob(JobBase* job);

	// Vulkan queue submissions can only be done from one thread (each queue must have its own thread)
	void addGPUJob(JobBase* job);

	/// {{During rendering CPU to GPU memory transfer operation will be submitted to a separate transfer queue from the main thread}}
	/// NOTE: for now this is run on the GPU worker thread!!
	/// TODO: consider if there is any benefit in this running on the main thread
	/// void addGPUTransferJob(JobBase* job);

	// DISK <-> RAM transfer operation will have their own thread
	void addDiskIOJob(JobBase* job);

	// Mark job for freeing memory
	void freeJob(JobBase* job);

	// Free jobs that are finished
	void freeFinishedJobs();

	// A job that calls freeFinishedJobs
	static void cleanupJob();

	std::list<JobBase*> m_jobsToFree;
	std::mutex m_jobsFreeMutex;


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
protected:

	JobBase(Threading::WorkerThread* owningWorker) : m_owningWorker(owningWorker) {}
	JobBase() {}
	JobBase(const JobBase&) = delete;

public:

	virtual void run() = 0;

	void setChild(JobBase* pChild)
	{
		child = pChild;
	}

protected:

	void pushChildJob() { 
		child->m_owningWorker->pushJob(child); 
	}

	Threading::WorkerThread* m_owningWorker = nullptr;
	JobBase * child = nullptr;
};

template<typename JobFuncType = VoidJobType>
class Job : public JobBase
{
	friend class Threading;
public:
	/*
		Constructor for regular jobs
	*/
	Job(JobFuncType pJobFunction) : jobFunction(pJobFunction) {}

	/*
		This constructor is for child jobs that would be run on a different worker thread than the parent job.
		Usually m_owningThread is set automatically when a job is pushed to a specific worker,
		however for child jobs that need to be run by a different worker, this constructor should be used
		so that the child job is pushed to the correct worker thread
	*/
	Job(JobFuncType pJobFunction, Threading::WorkerThread* owningWorker) : jobFunction(pJobFunction), JobBase(owningWorker) {}

	void run()
	{
		jobFunction();

		if (child) {
			pushChildJob();
		}

		Engine::threading->freeJob(this);
	}

private:

	JobFuncType jobFunction;
};




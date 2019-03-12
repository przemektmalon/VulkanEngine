#include "PCH.hpp"
#include "Threading.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Profiler.hpp"

Threading::Threading(int pNumThreads)
{
	/// TODO: spawn extra threads from pNumThreads ?
	initCompulsoryWorkers();
}

Threading::~Threading()
{
	for (auto w : workerThreads) {
		if (w)
			w->join();
	}
}

void Threading::addCPUJob(JobBase * jobToAdd)
{
	cpuWorker->pushJob(jobToAdd);
}

void Threading::addGPUJob(JobBase * jobToAdd)
{
	gpuWorker->pushJob(jobToAdd);
}

void Threading::addCPUTransferJob(JobBase * jobToAdd)
{
	diskIOWorker->pushJob(jobToAdd);
}

void Threading::initCompulsoryWorkers()
{
	// Create CPU worker
	cpuWorker = new WorkerThread(
		[]()->void { 
			Engine::renderer->createPerThreadCommandPools(); 
		},
		[this]()->void {
			while (gpuWorker)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
			vkQueueWaitIdle(Engine::renderer->lGraphicsQueue.getHandle());
			vkQueueWaitIdle(Engine::renderer->lTransferQueue.getHandle());
			Engine::renderer->commandPool.destroy();
		},
		"cpu");
	
	// Create GPU worker
	gpuWorker = new WorkerThread(
		[]()->void { Engine::renderer->createPerThreadCommandPools(); },
		[this]()->void {
			/// TODO: this shouldn't be neccessary ? But if we get mysterious crashes then we will have to find a work around 
			/// Maybe a boolean in the ThreadWorker to indicate whether to wait for all jobs to finish
			/// However some jobs (CPU) instantly add a child upon finishing, so this would forever block thread closing in that case
			//while (NOT ALL GPU JOBS DONE)
			//{
			//	std::this_thread::sleep_for(std::chrono::milliseconds(5));
			//}
			Engine::renderer->executeFenceDelayedActions();
			Engine::renderer->lGraphicsQueue.waitIdle();
			Engine::renderer->lTransferQueue.waitIdle();
			Engine::renderer->commandPool.destroy();
			gpuWorker = nullptr;
		},
		"gpu");

	// Create DISK IO worker
	diskIOWorker = new WorkerThread(
		[]()->void { },
		[]()->void { },
		"disk");

	// Add the created workers to the workers list
	workerThreads.push_back(cpuWorker);
	workerThreads.push_back(gpuWorker);
	workerThreads.push_back(diskIOWorker);
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

void Threading::WorkerThread::pushJob(JobBase * job)
{
	//std::unique_lock lock{ m_jobsQueueMutex, std::adopt_lock };

	m_jobsQueueMutex.lock();

	job->owningWorker = this;
	++m_totalJobsAdded;
	m_jobsQueue.push(job);

	m_jobsQueueMutex.unlock();
}

bool Threading::WorkerThread::popJob(JobBase *& job)
{
	//std::unique_lock lock{ m_jobsQueueMutex, std::adopt_lock };

	m_jobsQueueMutex.lock();

	if (m_jobsQueue.empty()) {
		m_jobsQueueMutex.unlock();
		return false;
	}
	else {
		job = m_jobsQueue.front();
		m_jobsQueue.pop();
	}
	m_jobsQueueMutex.unlock();
	return true;
}

void Threading::WorkerThread::run()
{
	// Register the thread in the engine profiler
	Engine::waitForProfilerInitMutex.lock();
	Engine::waitForProfilerInitMutex.unlock();
	Engine::threading->initThreadProfilerTagsMutex.lock();
	Profiler::prealloc(std::vector<std::thread::id>{ std::this_thread::get_id() }, std::vector<std::string>{ "thread_" + getThisThreadIDString() });
	Engine::threading->initThreadProfilerTagsMutex.unlock();

	m_initFunc();

	JobBase* job;
	bool readyToTerminate = false;
	//s64 timeUntilNextJob = std::numeric_limits<s64>::max();
	while (!readyToTerminate)
	{
		if (popJob(job))
		{
			PROFILE_START("thread_" + getThisThreadIDString());
			Engine::renderer->executeFenceDelayedActions(); // Any externally synced vulkan objects created on this thread should be destroyed from this thread
			job->run();
			m_totalJobsFinished++;
			PROFILE_END("thread_" + getThisThreadIDString());
		}
		else
		{
			readyToTerminate = !Engine::engineRunning;
			std::this_thread::yield();
		}
	}

	m_closeFunc();
}

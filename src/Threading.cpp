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
	for (auto w : m_workerThreads) {
		if (w)
			w->join();
	}
}

std::string Threading::getThisThreadIDString()
{
	std::stringstream ss;
	ss << std::this_thread::get_id();
	return ss.str();
}

std::string Threading::getThreadIDString(std::thread::id id)
{
	std::stringstream ss;
	ss << id;
	return ss.str();
}

void Threading::addCPUJob(JobBase * jobToAdd)
{
	m_cpuWorker->pushJob(jobToAdd);
}

void Threading::addGPUJob(JobBase * jobToAdd)
{
	m_gpuWorker->pushJob(jobToAdd);
}

void Threading::addDiskIOJob(JobBase * jobToAdd)
{
	m_diskIOWorker->pushJob(jobToAdd);
}

void Threading::initCompulsoryWorkers()
{
	// Create CPU worker
	m_cpuWorker = new WorkerThread(
		[]()->void { 
			Engine::renderer->createPerThreadCommandPools(); 
		},
		[this]()->void {
			while (m_gpuWorker)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
			vkQueueWaitIdle(Engine::renderer->lGraphicsQueue.getHandle());
			vkQueueWaitIdle(Engine::renderer->lTransferQueue.getHandle());
			Engine::renderer->commandPool.destroy();
		},
		"cpu");
	
	// Create GPU worker
	m_gpuWorker = new WorkerThread(
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
			m_gpuWorker = nullptr;
		},
		"gpu");

	// Create DISK IO worker
	m_diskIOWorker = new WorkerThread(
		[]()->void { },
		[]()->void { },
		"disk");

	// Add the created workers to the workers list
	m_workerThreads.push_back(m_cpuWorker);
	m_workerThreads.push_back(m_gpuWorker);
	m_workerThreads.push_back(m_diskIOWorker);
}

void Threading::freeJob(JobBase * jobToFree)
{
	m_jobsFreeMutex.lock();
	m_jobsToFree.push_front(jobToFree);
	m_jobsFreeMutex.unlock();
}

void Threading::freeFinishedJobs()
{
	m_jobsFreeMutex.lock();
	for (auto j : m_jobsToFree)
	{
		delete j;
	}
	m_jobsToFree.clear();
	m_jobsFreeMutex.unlock();
}

void Threading::cleanupJob()
{
	Engine::threading->freeFinishedJobs();

	if (Engine::engineRunning)
	{
		auto nextCleanupJob = new Job<>(&cleanupJob);
		//nextCleanupJob->setScheduledTime(Engine::clock.now() + 10000);
		Engine::threading->addCPUJob(nextCleanupJob);
	}
}

void Threading::WorkerThread::pushJob(JobBase * job)
{
	//std::unique_lock lock{ m_jobsQueueMutex, std::adopt_lock };

	m_jobsQueueMutex.lock();

	job->m_owningWorker = this;
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
	Engine::threading->m_initThreadProfilerTagsMutex.lock();
	Profiler::prealloc(std::vector<std::thread::id>{ std::this_thread::get_id() }, std::vector<std::string>{ "thread_" + getThisThreadIDString() });
	Engine::threading->m_initThreadProfilerTagsMutex.unlock();

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

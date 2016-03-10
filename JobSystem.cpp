#if 0
#include "JobSystem.h"
#include <atomic>
#include <thread>

// Assuming 64 bit platform
// Fits a cache line
struct Job
{
	JobFunction* function;
	Job* parent;
	std::atomic_int32_t unfinishedJobs;
	char padding[44]; // 64 - 8 - 8 - 4
	void* data;
};

struct WorkStealingQueue {
	Job* Steal();
	Job* Pop();
	void Push(Job*);
};

#define JOB_FUNCTION(name) void name(Job*, const void*)
typedef JOB_FUNCTION(JobFunction);

static Job** g_jobsToDelete;
static WorkStealingQueue** g_jobQueues;
static uint32_t g_workerThreadCount = std::thread::hardware_concurrency();
static std::atomic_int32_t g_jobToDeleteCount;



// Define the empty job
JOB_FUNCTION(empty_job) {}

Job* GetJob(void)
{
	WorkStealingQueue* queue = GetWorkerThreadQueue();

	Job* job = queue->Pop();
	if (job->function == &empty_job)
	{
		// this is not a valid job because our own queue is empty, so try stealing from some other queue
		unsigned int randomIndex = GenerateRandomNumber(0, g_workerThreadCount + 1);
		WorkStealingQueue* stealQueue = g_jobQueues[randomIndex];
		if (stealQueue == queue)
		{
			// don't try to steal from ourselves
			std::this_thread::yield();
			return nullptr;
		}

		Job* stolenJob = stealQueue->Steal();
		if (stolenJob->function == &empty_job)
		{
			// we couldn't steal a job from the other queue either, so we just yield our time slice for now
			std::this_thread::yield();
			return nullptr;
		}

		return stolenJob;
	}

	return job;
}

void Execute(Job* job)
{
	(job->function)(job, job->data);
	Finish(job);
}

Job* CreateJob(JobFunction* function)
{
	Job* job = new Job;
	job->function = function;
	job->parent = nullptr;
	job->unfinishedJobs = 1;

	return job;
}

Job* CreateJobAsChild(Job* parent, JobFunction* function)
{
	++parent->unfinishedJobs;

	Job* job = new Job;
	job->function = function;
	job->parent = parent;
	job->unfinishedJobs = 1;

	return job;
}

void Run(Job* job)
{
	WorkStealingQueue* queue = GetWorkerThreadQueue();
	queue->Push(job);
}

void Wait(const Job* job)
{
	// wait until the job has completed. in the meantime, work on any other job.
	while (!HasJobCompleted(job))
	{
		Job* nextJob = GetJob();
		if (nextJob)
		{
			Execute(nextJob);
		}
	}
}

void Finish(Job* job)
{
	const int32_t unfinishedJobs = --job->unfinishedJobs;
	if (unfinishedJobs == 0)
	{
		const int32_t index = ++g_jobToDeleteCount;
		g_jobsToDelete[index - 1] = job;

		if (job->parent)
		{
			Finish(job->parent);
		}

		--job->unfinishedJobs;
	}
}

void WorkerThread() {
	while (true)
	{
		Job* job = GetJob();
		if (job)
		{
			Execute(job);
		}
	}
}

void Test() {
	Job* root = CreateJob(&empty_job);
	for (unsigned int i = 0; i < g_workerThreadCount; ++i)
	{
		Job* job = CreateJobAsChild(root, &empty_job);
		Run(job);
	}
	Run(root);
	Wait(root);
}
#endif

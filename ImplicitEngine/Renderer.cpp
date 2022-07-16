#include "Renderer.h"

Renderer::Renderer(CallbackFun refreshCallback_)
	: refreshCallback(refreshCallback_),
	jobPollThread(&Renderer::JobPollLoop, std::ref(*this))
{}

Renderer::~Renderer()
{
	active = false;
	jobPollThread.join();

	for (auto& job : jobs)
		delete job;
}

void Renderer::JobPollLoop()
{
	bool allComplete = false;
	while (active)
	{
		bool outdatedJobs = false;

		// Thread-safe for-loop over jobs
		int jobIndex = 0;
		for (;;)
		{
			Job* job = nullptr;
			jobMutex.lock();
			if (jobIndex < jobs.size())
			{
				job = jobs[jobIndex];
				jobIndex++;
			}
			else job = nullptr;
			jobMutex.unlock();

			if (!job) break;

			if (job->status == JobStatus::OUTDATED)
			{
				outdatedJobs = true;
				allComplete = false;

				job->status = JobStatus::PROCESSING;
				ProcessJob(job);
				job->bufferedVerts = job->verts;
				if (job->status == JobStatus::PROCESSING)
					job->status == JobStatus::COMPLETE;

				break;
			}
		}

		if (!outdatedJobs && !allComplete)
		{
			allComplete = true;
			refreshCallback();
		}
	}
}

void Renderer::NewJob(std::string_view funcStr, const Bounds& bounds, size_t id)
{
	jobMutex.lock();
	jobs.push_back(new Job(funcStr, bounds, id));
	jobMutex.unlock();
}

void Renderer::EditJob(size_t id, std::string_view newFunc)
{
	jobMutex.lock();
	Job* job = *std::find_if(jobs.begin(), jobs.end(), [id](Job* job) { return job->jobID == id; });
	job->func.Construct(newFunc);
	job->status = JobStatus::OUTDATED;
	jobMutex.unlock();
}

void Renderer::DeleteJob(size_t id)
{
	jobMutex.lock();
	auto pos = std::find_if(jobs.begin(), jobs.end(), [id](Job* job) { return job->jobID == id; });
	Job* job = *pos;
	jobs.erase(pos);
	jobMutex.unlock();

	delete job;
}
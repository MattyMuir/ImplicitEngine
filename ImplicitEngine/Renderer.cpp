#include "Renderer.h"

Renderer::Renderer(CallbackFun refreshCallback_)
	: refreshCallback(refreshCallback_),
	jobPollThread(&Renderer::JobPollLoop, std::ref(*this))
{}

Renderer::~Renderer()
{
	active = false;
	jobPollThread.join();

	for (Job* job : jobs)
		delete job;
}

void Renderer::JobPollLoop()
{
	bool allComplete = false;
	while (active)
	{
		bool outdatedJobs = false;

		jobMutex.lock();
		for (Job* job : jobs)
		{
			if (job->status == JobStatus::OUTDATED)
			{
				outdatedJobs = true;
				allComplete = false;

				ProcessJob(job);
				break;
			}
		}
		jobMutex.unlock();

		if (!outdatedJobs && !allComplete)
		{
			allComplete = true;
			refreshCallback();
		}
	}
}

void Renderer::NewJob(const Function& func, const Bounds& bounds, size_t id)
{
	jobMutex.lock();
	jobs.push_back(new Job(func, bounds, id));
	jobMutex.unlock();
}

void Renderer::DeleteJob(size_t id)
{
	jobMutex.lock();
	auto pos = std::find_if(jobs.begin(), jobs.end(), [id](Job* job) { return job->jobID == id; });
	Job* ptr = *pos;
	jobs.erase(pos);
	jobMutex.unlock();

	delete ptr;
}
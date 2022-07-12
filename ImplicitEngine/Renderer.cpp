#include "Renderer.h"

Renderer::Renderer(CallbackFun refreshCallback_)
	: refreshCallback(refreshCallback_),
	jobPollThread(&Renderer::JobPollLoop, std::ref(*this))
{}

Renderer::~Renderer()
{
	active = false;
	jobPollThread.join();
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
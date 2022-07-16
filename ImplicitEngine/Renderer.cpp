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

		// Lock-free, thread-safe loop over jobs
		decltype(jobs)::iterator it = jobs.begin();
		while (it != jobs.end())
		{
			std::shared_ptr<Job> job = *it;

			if (job->status == JobStatus::OUTDATED)
			{
				outdatedJobs = true;
				allComplete = false;

				job->status = JobStatus::PROCESSING;
				ProcessJob(job.get());
				job->bufferedVerts = job->verts;
				if (job->status == JobStatus::PROCESSING)
					job->status == JobStatus::COMPLETE;

				break;
			}

			it++;
		}

		if (deleteList.size() > 0)
		{
			deleteMutex.lock();
			for (size_t id : deleteList)
			{
				auto pos = std::find_if(jobs.begin(), jobs.end(), [id](std::shared_ptr<Job> job) { return job->jobID == id; });
				jobs.erase(pos);
			}

			deleteList.clear();
			deleteMutex.unlock();

			refreshCallback();
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
	jobs.push_back(std::make_shared<Job>(funcStr, bounds, id));
}

void Renderer::EditJob(size_t id, std::string_view newFunc)
{
	std::shared_ptr<Job> job = *std::find_if(jobs.begin(), jobs.end(), [id](std::shared_ptr<Job> job) { return job->jobID == id; });
	job->func.Construct(newFunc);
	job->status = JobStatus::OUTDATED;
}

void Renderer::DeleteJob(size_t id)
{
	deleteMutex.lock();
	deleteList.push_back(id);
	deleteMutex.unlock();
}
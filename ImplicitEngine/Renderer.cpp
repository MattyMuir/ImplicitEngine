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

		/*// Thread-safe for-loop over jobs
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
		}*/

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
	auto pos = std::find_if(jobs.begin(), jobs.end(), [id](std::shared_ptr<Job> job) { return job->jobID == id; });
	std::shared_ptr<Job> job = *pos;
	jobs.erase(pos);
}
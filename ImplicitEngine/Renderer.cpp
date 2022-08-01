#include "Renderer.h"

Renderer::Renderer(CallbackFun refreshCallback_)
	: refreshCallback(refreshCallback_), pollingBar(2),
	jobPollThread(&Renderer::JobPollLoop, this, std::stop_token{})
{}

Renderer::~Renderer()
{
	jobPollThread.request_stop();
	SignalJobRescan(); // Allow the poller to break from idle
	jobPollThread.join();
}

void Renderer::JobPollLoop(std::stop_token token)
{
	bool allComplete = false;
	while (!token.stop_requested())
	{
		if (allComplete)
		{
			// Spin idly until signalled to re-check
			std::cout << "Spinning...\n";
			pollingBar.arrive_and_wait();
			std::cout << "Stopped spinning.\n";
		}

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

				std::unique_lock lock(job->bufferMutex);
				job->bufferedVerts = job->verts;
				lock.unlock();

				if (job->status == JobStatus::PROCESSING)
					job->status = JobStatus::COMPLETE;

				break;
			}

			it++;
		}

		if (deleteList.size() > 0)
		{
			std::unique_lock lock(deleteMutex);
			for (size_t id : deleteList)
			{
				auto pos = std::find_if(jobs.begin(), jobs.end(), [id](std::shared_ptr<Job> job) { return job->id == id; });
				jobs.erase(pos);
			}

			deleteList.clear();
			lock.unlock();

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
	SignalJobRescan();
}

void Renderer::EditJob(size_t id, std::string_view newFunc)
{
	std::shared_ptr<Job> job = *std::find_if(jobs.begin(), jobs.end(), [id](std::shared_ptr<Job> job) { return job->id == id; });
	job->funcs.Change(newFunc);
	job->status = JobStatus::OUTDATED;
}

void Renderer::DeleteJob(size_t id)
{
	std::lock_guard lock(deleteMutex);
	deleteList.push_back(id);
}

void Renderer::UpdateJobs()
{
	for (std::shared_ptr<Job> job : jobs)
		job->status = JobStatus::OUTDATED;

	SignalJobRescan();
}

void Renderer::SignalJobRescan()
{
	pollingBar.arrive();
}
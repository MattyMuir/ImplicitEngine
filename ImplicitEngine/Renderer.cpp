#include "Renderer.h"

Renderer::Renderer(CallbackFun refreshCallback_)
	: pollingBar(2), refreshCallback(refreshCallback_),
	jobPollThread(&Renderer::JobPollLoop, this)
{}

Renderer::~Renderer()
{
	jobPollThread.request_stop();
	SignalJobRescan(); // Allow the poller to break from idle
	jobPollThread.join();
}

void Renderer::JobPollLoop()
{
	std::stop_token token = jobPollThread.get_stop_token();

	bool allComplete = false;
	while (!token.stop_requested())
	{
		if (allComplete)
		{
			// Spin idly until signalled to re-check
			pollingBar.arrive_and_wait();
		}

		bool outdatedJobs = false;

		// Lock-free, thread-safe loop over jobs
		decltype(jobs)::iterator it = jobs.begin();
		while (it != jobs.end())
		{
			std::shared_ptr<Job> job = *it;

			if (job->isValid && job->status == JobStatus::OUTDATED)
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

bool Renderer::NewJob(std::string_view funcStr, const Bounds& bounds, size_t id, bool isValid)
{
	auto newJob = std::make_shared<Job>(funcStr, bounds, id);
	bool compValid = newJob->isValid;

	newJob->isValid &= isValid;
	jobs.push_back(newJob);

	SignalJobRescan();
	return compValid;
}

bool Renderer::EditJob(size_t id, std::string_view newFunc, bool isValid)
{
	std::shared_ptr<Job> job = *std::find_if(jobs.begin(), jobs.end(), [id](std::shared_ptr<Job> job) { return job->id == id; });
	job->funcs.Change(newFunc);
	job->isValid = job->funcs.isValid;
	bool compValid = job->isValid;

	job->isValid &= isValid;
	job->status = JobStatus::OUTDATED;

	SignalJobRescan();
	return compValid;
}

void Renderer::DeleteJob(size_t id)
{
	std::lock_guard lock(deleteMutex);
	deleteList.push_back(id);

	SignalJobRescan();
}

void Renderer::SetJobColor(size_t id, wxColour col)
{
	std::shared_ptr<Job> job = *std::find_if(jobs.begin(), jobs.end(), [id](std::shared_ptr<Job> job) { return job->id == id; });
	job->col = col;

	job->status = JobStatus::OUTDATED;
	refreshCallback();
}

wxColour Renderer::GetJobColour(size_t id)
{
	std::shared_ptr<Job> job = *std::find_if(jobs.begin(), jobs.end(), [id](std::shared_ptr<Job> job) { return job->id == id; });
	return job->col;
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

Job::Job(std::string_view funcStr, const Bounds& bounds_, size_t id_)
	: bounds(bounds_), funcs(funcStr, 1), id(id_), col(0, 0, 0)
{
	isValid = funcs.isValid;
}
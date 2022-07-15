#pragma once
#include <vector>
#include <functional>
#include <thread>
#include <mutex>

#include "Bounds.h"
#include "Function.h"

enum class JobStatus { OUTDATED, PROCESSING, COMPLETE };
typedef std::function<void()> CallbackFun;

class Canvas;
class Main;

struct Job
{
	Job(const Function& func_, const Bounds& bounds_, size_t jobID_)
		: func(func_), bounds(bounds_), jobID(jobID_) {}

	JobStatus status = JobStatus::OUTDATED;
	Bounds bounds;
	Function func;
	std::vector<float> verts;
	size_t jobID;
};

class Renderer
{
public:
	Renderer(CallbackFun refreshCallback_);
	~Renderer();

	void JobPollLoop();
	void NewJob(const Function& func, const Bounds& bounds, size_t id);
	void DeleteJob(size_t id);

protected:
	virtual void ProcessJob(Job* job) = 0;

	std::vector<Job*> jobs;
	std::mutex jobMutex;
	CallbackFun refreshCallback;
	volatile bool active = true;

	std::thread jobPollThread;

	friend Canvas;
	friend Main;
};
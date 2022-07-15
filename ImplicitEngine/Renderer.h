#pragma once
#include <iostream>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>

#include "Bounds.h"
#include "Function.h"
#include "Timer.h"

enum class JobStatus { OUTDATED, PROCESSING, COMPLETE };
typedef std::function<void()> CallbackFun;

class Canvas;
class Main;

struct Job
{
	Job(std::string_view funcStr, const Bounds& bounds_, size_t jobID_)
		: func(funcStr), bounds(bounds_), jobID(jobID_) {}

	JobStatus status = JobStatus::OUTDATED;
	Bounds bounds;
	Function func;
	std::vector<double> verts, bufferedVerts;
	size_t jobID;
};

class Renderer
{
public:
	Renderer(CallbackFun refreshCallback_);
	~Renderer();

	void JobPollLoop();
	void NewJob(std::string_view funcStr, const Bounds& bounds, size_t id);
	void EditJob(size_t id, std::string_view newFunc);
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
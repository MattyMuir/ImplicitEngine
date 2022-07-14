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

struct Job
{
	JobStatus status = JobStatus::OUTDATED;
	Bounds bounds;
	Function func;
	std::vector<float> verts;
};

class Renderer
{
public:
	Renderer(CallbackFun refreshCallback_);
	~Renderer();

	void JobPollLoop();

protected:
	virtual void ProcessJob(Job* job) = 0;

	std::vector<Job*> jobs;
	std::mutex jobMutex;
	CallbackFun refreshCallback;
	volatile bool active = true;

	std::thread jobPollThread;

	friend Canvas;
};
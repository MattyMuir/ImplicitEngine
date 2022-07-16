#pragma once
#include <iostream>
#include <list>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>

#include "Bounds.h"
#include "Timer.h"
#include "FunctionPack.h"

enum class JobStatus { OUTDATED, PROCESSING, COMPLETE };
typedef std::function<void()> CallbackFun;

class Canvas;
class Main;

struct Job
{
	Job(std::string_view funcStr, const Bounds& bounds_, size_t jobID_)
		: funcs(funcStr, 1), bounds(bounds_), jobID(jobID_) {}

	JobStatus status = JobStatus::OUTDATED;
	Bounds bounds;
	FunctionPack funcs;
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

	std::list<std::shared_ptr<Job>> jobs;
	std::mutex deleteMutex;
	std::list<size_t> deleteList;
	CallbackFun refreshCallback;
	volatile bool active = true;

	std::thread jobPollThread;

	friend Canvas;
	friend Main;
};
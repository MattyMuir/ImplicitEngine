#pragma once
#include <iostream>
#include <list>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <barrier>

#include <wx/colour.h>

#include "Bounds.h"
#include "Timer.h"
#include "FunctionPack.h"

enum class JobStatus { OUTDATED, PROCESSING, COMPLETE };
typedef std::function<void()> CallbackFun;

class Canvas;
class Main;

struct Job
{
	Job(std::string_view funcStr, const Bounds& bounds_, size_t id_);

	JobStatus status = JobStatus::OUTDATED;
	Bounds bounds;
	FunctionPack funcs;
	std::vector<double> verts, bufferedVerts;
	std::mutex bufferMutex;
	size_t id;
	wxColour col;
	bool isValid;
};

class Renderer
{
public:
	Renderer(CallbackFun refreshCallback_);
	~Renderer();

	void JobPollLoop();

	bool NewJob(std::string_view funcStr, const Bounds& bounds, size_t id, bool isValid);
	bool EditJob(size_t id, std::string_view newFunc, bool isValid);
	void DeleteJob(size_t id);
	void SetJobColor(size_t id, wxColour col);
	wxColour GetJobColour(size_t id);

	void UpdateJobs();
	void SignalJobRescan();

protected:
	virtual void ProcessJob(Job* job) = 0;

	std::list<std::shared_ptr<Job>> jobs;
	std::barrier<> pollingBar;
	std::mutex deleteMutex;
	std::list<size_t> deleteList;
	CallbackFun refreshCallback;

	std::jthread jobPollThread;

	friend Canvas;
	friend Main;
};
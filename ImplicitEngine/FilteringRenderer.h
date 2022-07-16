#pragma once
#include <BS_thread_pool.hpp>
#include <ctpl_stl.h>

#include "Renderer.h"
#include "ProximalBracketingGenerator.h"

typedef BS::thread_pool ThreadPool;

class FilteringRenderer : public Renderer
{
public:
	FilteringRenderer(CallbackFun refreshFun, int seedNum_ = 100000, int filterMeshRes_ = 5, int finalMeshRes_ = 9);

	~FilteringRenderer();

protected:
	void ProcessJob(Job* job);

	ThreadPool pool;

	int seedNum;
	int filterMeshRes;
	int finalMeshRes;

	std::vector<std::vector<Seed>> seeds;
};